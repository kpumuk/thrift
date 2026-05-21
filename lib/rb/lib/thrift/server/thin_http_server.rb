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

require 'delegate'
require 'rack'
require 'thin'

##
# Wraps the Thin web server to provide a Thrift server over HTTP.
module Thrift
  class ThinHTTPServer < BaseServer

    ##
    # Accepts a Thrift::Processor
    # Options include:
    # * :port
    # * :ip
    # * :path
    # * :protocol_factory
    def initialize(processor, options = {})
      port = options[:port] || 80
      ip = options[:ip] || "0.0.0.0"
      path = options[:path] || "/"
      protocol_factory = options[:protocol_factory] || BinaryProtocolFactory.new
      app = RackApplication.for(path, processor, protocol_factory)
      @server = Thin::Server.new(ip, port, app)
    end

    ##
    # Starts the server
    def serve
      @server.start
    end

    class RackApplication

      THRIFT_HEADER = "application/x-thrift"
      RESPONSE_HEADERS = {'Content-Type' => THRIFT_HEADER}.freeze
      ONEWAY_RESPONSE_HEADERS = RESPONSE_HEADERS.merge('Content-Length' => '0').freeze

      def self.for(path, processor, protocol_factory)
        Rack::Builder.new do
          use Rack::CommonLogger
          use Rack::ShowExceptions
          use Rack::Lint
          map path do
            run lambda { |env|
              request = Rack::Request.new(env)
              if RackApplication.valid_thrift_request?(request)
                if RackApplication.async_request?(env)
                  RackApplication.process_async_request(request, processor, protocol_factory, env['async.callback'])
                  throw :async
                end

                RackApplication.successful_request(request, processor, protocol_factory).response.finish
              else
                RackApplication.failed_request.finish
              end
            }
          end
        end
      end

      def self.async_request?(env)
        env['async.callback']
      end

      def self.process_async_request(rack_request, processor, protocol_factory, callback)
        Thread.new do
          result = nil
          result = successful_request(rack_request, processor, protocol_factory, callback)
          async_callback(callback, result.response.finish) unless result.response_sent?
        rescue StandardError
          unless result && result.response_sent?
            async_callback(callback, [500, {'Content-Type' => 'text/plain', 'Content-Length' => '21'}, ['Internal Server Error']])
          end
        end
      end

      def self.async_callback(callback, response)
        if defined?(EM) && EM.reactor_running?
          EM.schedule { callback.call(response) }
        else
          callback.call(response)
        end
      end

      def self.successful_request(rack_request, processor, protocol_factory, async_callback = nil)
        response = Rack::Response.new([], 200, RESPONSE_HEADERS)
        result = RequestResult.new(response)
        transport = ResponseTransport.new(rack_request.body, response)
        protocol = OnewayResponseProtocol.new(protocol_factory.get_protocol(transport)) do
          unless result.response_sent?
            result.mark_response_sent!
            transport.response_sent!
            if async_callback
              RackApplication.async_callback(async_callback, [200, ONEWAY_RESPONSE_HEADERS, []])
            else
              response.status = 200
              response.headers.replace(ONEWAY_RESPONSE_HEADERS)
              response.body = []
            end
          end
        end
        processor.process protocol, protocol
        result
      end

      def self.failed_request
        Rack::Response.new(['Not Found'], 404, RESPONSE_HEADERS)
      end

      def self.valid_thrift_request?(rack_request)
        rack_request.post? && rack_request.env["CONTENT_TYPE"] == THRIFT_HEADER
      end

      class RequestResult
        attr_reader :response

        def initialize(response)
          @response = response
          @response_sent = false
        end

        def mark_response_sent!
          @response_sent = true
        end

        def response_sent?
          @response_sent
        end
      end

      class ResponseTransport < IOStreamTransport
        def initialize(input, output)
          super
          @response_sent = false
        end

        def response_sent!
          @response_sent = true
        end

        def write(buf)
          return if @response_sent

          super
        end
      end

      class OnewayResponseProtocol < SimpleDelegator
        def initialize(protocol, &callback)
          super(protocol)
          @callback = callback
          @oneway_message = false
        end

        def read_message_begin
          message_begin = __getobj__.read_message_begin
          @oneway_message ||= message_begin[1] == MessageTypes::ONEWAY
          message_begin
        end

        def read_message_end
          __getobj__.read_message_end.tap do
            if @oneway_message
              @oneway_message = false
              @callback.call
            end
          end
        end
      end

    end

  end
end
