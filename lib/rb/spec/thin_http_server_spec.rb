# frozen_string_literal: true
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

require 'spec_helper'
require 'net/http'
require 'rack/test'
require 'socket'
require 'timeout'
require 'thrift/server/thin_http_server'

describe Thrift::ThinHTTPServer do
  let(:processor) { double('processor') }

  describe "#initialize" do
    context "when using the defaults" do
      it "binds to port 80, with host 0.0.0.0, a path of '/'" do
        expect(Thin::Server).to receive(:new).with('0.0.0.0', 80, an_instance_of(Rack::Builder))
        Thrift::ThinHTTPServer.new(processor)
      end

      it 'creates a ThinHTTPServer::RackApplicationContext' do
        expect(Thrift::ThinHTTPServer::RackApplication).to receive(:for).with("/", processor, an_instance_of(Thrift::BinaryProtocolFactory)).and_return(anything)
        Thrift::ThinHTTPServer.new(processor)
      end

      it "uses the BinaryProtocolFactory" do
        expect(Thrift::BinaryProtocolFactory).to receive(:new)
        Thrift::ThinHTTPServer.new(processor)
      end
    end

    context "when using the options" do
      it 'accepts :ip, :port, :path' do
        ip = "192.168.0.1"
        port = 3000
        path = "/thin"
        expect(Thin::Server).to receive(:new).with(ip, port, an_instance_of(Rack::Builder))
        Thrift::ThinHTTPServer.new(processor,
                           :ip => ip,
                           :port => port,
                           :path => path)
      end

      it 'creates a ThinHTTPServer::RackApplicationContext with a different protocol factory' do
        expect(Thrift::ThinHTTPServer::RackApplication).to receive(:for).with("/", processor, an_instance_of(Thrift::JsonProtocolFactory)).and_return(anything)
        Thrift::ThinHTTPServer.new(processor,
                           :protocol_factory => Thrift::JsonProtocolFactory.new)
      end
    end
  end

  describe "#serve" do
    it 'starts the Thin server' do
      underlying_thin_server = double('thin server', :start => true)
      allow(Thin::Server).to receive(:new).and_return(underlying_thin_server)

      thin_thrift_server = Thrift::ThinHTTPServer.new(processor)

      expect(underlying_thin_server).to receive(:start)
      thin_thrift_server.serve
    end
  end
end

describe Thrift::ThinHTTPServer::RackApplication do
  include Rack::Test::Methods

  let(:processor) { double('processor') }
  let(:protocol_factory) { double('protocol factory') }
  let(:protocol) { FakeProtocol.new }

  def app
    Thrift::ThinHTTPServer::RackApplication.for("/", processor, protocol_factory)
  end

  context "404 response" do
    it 'receives a non-POST' do
      header('Content-Type', "application/x-thrift")
      get "/"
      expect(last_response.status).to eq 404
    end

    it 'receives a header other than application/x-thrift' do
      header('Content-Type', "application/json")
      post "/"
      expect(last_response.status).to eq 404
    end
  end

  context "200 response" do
    before do
      allow(protocol_factory).to receive(:get_protocol).and_return(protocol)
      allow(processor).to receive(:process)
    end

    it 'creates an IOStreamTransport' do
      header('Content-Type', "application/x-thrift")
      expect(Thrift::ThinHTTPServer::RackApplication::ResponseTransport).to receive(:new).with(an_instance_of(Rack::Lint::InputWrapper), an_instance_of(Rack::Response)).and_call_original
      post "/"
    end

    it 'fetches the right protocol based on the Transport' do
      header('Content-Type', "application/x-thrift")
      expect(protocol_factory).to receive(:get_protocol).with(an_instance_of(Thrift::ThinHTTPServer::RackApplication::ResponseTransport))
      post "/"
    end

    it 'status code 200' do
      header('Content-Type', "application/x-thrift")
      post "/"
      expect(last_response.ok?).to be true
    end

    it 'returns an empty response for oneway requests' do
      allow(processor).to receive(:process) do |iprot, _oprot|
        iprot.read_message_begin
        iprot.read_message_end
      end

      header('Content-Type', "application/x-thrift")
      post "/"

      expect(last_response.ok?).to be true
      expect(last_response.headers['Content-Length']).to eq('0')
      expect(last_response.body).to eq('')
    end

    it 'sends an async oneway response before processing completes' do
      response_queue = Queue.new
      finish_queue = Queue.new
      request = Rack::Request.new(
        'rack.input' => StringIO.new(''),
        'REQUEST_METHOD' => 'POST',
        'CONTENT_TYPE' => 'application/x-thrift'
      )

      allow(processor).to receive(:process) do |iprot, _oprot|
        iprot.read_message_begin
        iprot.read_message_end
        finish_queue.pop
      end

      thread = described_class.process_async_request(
        request,
        processor,
        protocol_factory,
        proc { |response| response_queue << response }
      )

      response = Timeout.timeout(1) { response_queue.pop }
      expect(response).to eq([200, {'Content-Type' => 'application/x-thrift', 'Content-Length' => '0'}, []])
      expect(thread).to be_alive

      finish_queue << true
      thread.join(1)
      expect(thread).not_to be_alive
    end

    it 'detects oneway message headers for supported HTTP protocols' do
      [
        Thrift::BinaryProtocol,
        Thrift::CompactProtocol,
        Thrift::JsonProtocol
      ].each do |protocol_class|
        trans = Thrift::MemoryBufferTransport.new
        protocol_class.new(trans).tap do |oprot|
          oprot.write_message_begin('testOneway', Thrift::MessageTypes::ONEWAY, 1)
          oprot.write_message_end
        end
        payload = trans.instance_variable_get(:@buf).dup

        detected_type = nil
        itrans = Thrift::MemoryBufferTransport.new(payload)
        iprot = described_class::OnewayResponseProtocol.new(protocol_class.new(itrans)) do
          detected_type = Thrift::MessageTypes::ONEWAY
        end

        iprot.read_message_begin
        iprot.read_message_end
        expect(detected_type).to eq(Thrift::MessageTypes::ONEWAY)
      end
    end

    it 'returns immediately from a real Thin server while the oneway handler continues' do
      processor = BlockingOnewayProcessor.new
      port = free_port
      server = Thrift::ThinHTTPServer.new(
        processor,
        :ip => '127.0.0.1',
        :port => port,
        :protocol_factory => Thrift::BinaryProtocolFactory.new
      )
      server_thread = Thread.new { server.serve }

      wait_until_http_server_listens(port)

      start = Time.now
      response = Net::HTTP.post(
        URI("http://127.0.0.1:#{port}/"),
        oneway_request_body,
        'Content-Type' => 'application/x-thrift'
      )
      elapsed = Time.now - start

      expect(response.code).to eq('200')
      expect(response.body).to eq('')
      expect(elapsed).to be < 0.2
      expect(processor.finished?).to be false

      processor.finish
      server.instance_variable_get(:@server).stop
      server_thread.join(1)
      expect(processor.finished?).to be true
    ensure
      processor&.finish
      server&.instance_variable_get(:@server)&.stop
      server_thread&.join(1)
    end

    it 'keeps a persistent HTTP connection usable after an early oneway response' do
      processor = PersistentOnewayProcessor.new
      port = free_port
      server = Thrift::ThinHTTPServer.new(
        processor,
        :ip => '127.0.0.1',
        :port => port,
        :protocol_factory => Thrift::BinaryProtocolFactory.new
      )
      server_thread = Thread.new { server.serve }

      wait_until_http_server_listens(port)

      socket = TCPSocket.new('127.0.0.1', port)
      socket.write(http_request(oneway_request_body))
      first_response = read_http_response(socket)
      expect(first_response).to include("HTTP/1.1 200 OK")
      expect(first_response).to include("Content-Length: 0")
      expect(processor.finished_count).to eq(0)

      socket.write(http_request(oneway_request_body))
      second_response = read_http_response(socket)
      expect(second_response).to include("HTTP/1.1 200 OK")
      expect(second_response).to include("Content-Length: 0")

      processor.finish_all
      server.instance_variable_get(:@server).stop
      server_thread.join(1)
      expect(processor.finished_count).to eq(2)
    ensure
      socket&.close
      processor&.finish_all
      server&.instance_variable_get(:@server)&.stop
      server_thread&.join(1)
    end
  end

  def free_port
    socket = TCPServer.new('127.0.0.1', 0)
    socket.addr[1]
  ensure
    socket&.close
  end

  def wait_until_http_server_listens(port)
    Timeout.timeout(2) do
      begin
        Net::HTTP.get_response(URI("http://127.0.0.1:#{port}/"))
      rescue Errno::ECONNREFUSED
        sleep 0.01
        retry
      end
    end
  end

  def oneway_request_body
    trans = Thrift::MemoryBufferTransport.new
    protocol = Thrift::BinaryProtocol.new(trans)
    protocol.write_message_begin('testOneway', Thrift::MessageTypes::ONEWAY, 1)
    protocol.write_struct_begin('TestOnewayArgs')
    protocol.write_field_stop
    protocol.write_struct_end
    protocol.write_message_end
    trans.instance_variable_get(:@buf).dup
  end

  def http_request(body)
    [
      "POST / HTTP/1.1",
      "Host: 127.0.0.1",
      "Content-Type: application/x-thrift",
      "Content-Length: #{body.bytesize}",
      "Connection: keep-alive",
      "",
      body
    ].join("\r\n")
  end

  def read_http_response(socket)
    Timeout.timeout(2) do
      buffer = ''.dup
      buffer << socket.readpartial(1024) until buffer.include?("\r\n\r\n")
      headers, rest = buffer.split("\r\n\r\n", 2)
      content_length = headers[/Content-Length:\s*(\d+)/i, 1].to_i
      while rest.bytesize < content_length
        rest << socket.readpartial(content_length - rest.bytesize)
      end
      "#{headers}\r\n\r\n#{rest}"
    end
  end

  class FakeProtocol
    attr_reader :trans

    def initialize(message_type = Thrift::MessageTypes::ONEWAY)
      @message_type = message_type
    end

    def read_message_begin
      ['testOneway', @message_type, 1]
    end

    def read_message_end
    end
  end

  class BlockingOnewayProcessor
    def initialize
      @finish_queue = Queue.new
      @finished = false
    end

    def process(iprot, _oprot)
      iprot.read_message_begin
      iprot.read_struct_begin
      _name, field_type, _field_id = iprot.read_field_begin
      raise "expected empty args struct" unless field_type == Thrift::Types::STOP
      iprot.read_struct_end
      iprot.read_message_end
      @finish_queue.pop
      @finished = true
      true
    end

    def finish
      @finish_queue << true
    end

    def finished?
      @finished
    end
  end

  class PersistentOnewayProcessor
    attr_reader :finished_count

    def initialize
      @finish_queue = Queue.new
      @finished_count = 0
    end

    def process(iprot, _oprot)
      iprot.read_message_begin
      iprot.read_struct_begin
      _name, field_type, _field_id = iprot.read_field_begin
      raise "expected empty args struct" unless field_type == Thrift::Types::STOP
      iprot.read_struct_end
      iprot.read_message_end
      @finish_queue.pop
      @finished_count += 1
      true
    end

    def finish_all
      2.times { @finish_queue << true }
    end
  end
end
