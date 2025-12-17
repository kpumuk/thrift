# Thrift Ruby Software Library

## License

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements. See the NOTICE file
distributed with this work for additional information
regarding copyright ownership. The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License. You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the License for the
specific language governing permissions and limitations
under the License.

# Using Thrift with Ruby

Ruby bindings for the Apache Thrift RPC system. The gem contains the runtime
types, transports, protocols, and servers used by generated Ruby code for both
clients and services.

## Compatibility

* Ruby MRI >= 2.4 (tested against current supported releases).
* JRuby works with the pure-Ruby implementation; the native extension is
  skipped automatically.

## Installation

* Requirements: Ruby >= 2.4.
* From RubyGems: `gem install thrift`
* From source: `bundle install` then `bundle exec rake gem` (builds the native
  accelerator where supported) and install the resulting `thrift-*.gem`.

## Generating Ruby Code

The Ruby library does not include the Thrift compiler. Use a compiler built
from the root of this repository to generate Ruby bindings:

    thrift --gen rb path/to/service.thrift
    # with namespaced modules
    thrift --gen rb:namespaced --recurse path/to/service.thrift

Generated files are typically written to `gen-rb/` and can be required
directly from your application.

## Basic Client Usage

    $:.push File.expand_path('gen-rb', __dir__)
    require 'thrift'
    require 'calculator'

    socket     = Thrift::Socket.new('localhost', 9090)
    transport  = Thrift::BufferedTransport.new(socket)
    protocol   = Thrift::BinaryProtocol.new(transport)
    client     = Calculator::Client.new(protocol)

    transport.open
    puts client.add(1, 1)
    transport.close

## Basic Server Usage

    $:.push File.expand_path('gen-rb', __dir__)
    require 'thrift'
    require 'calculator'

    class CalculatorHandler
      def add(a, b) = a + b
    end

    handler            = CalculatorHandler.new
    processor          = Calculator::Processor.new(handler)
    server_transport   = Thrift::ServerSocket.new(9090)
    transport_factory  = Thrift::BufferedTransportFactory.new
    protocol_factory   = Thrift::BinaryProtocolFactory.new

    server = Thrift::ThreadedServer.new(processor, server_transport,
                                        transport_factory, protocol_factory)
    server.serve

## Development and Tests

* `bundle exec rake spec` runs the Ruby specs. It expects a built Thrift
  compiler at `../../compiler/cpp/thrift`.
* `bundle exec rake test` runs the cross-language test suite; it must be
  executed from a full Thrift checkout.
* `bundle exec rake build_ext` (implicit in the tasks above) compiles the
  optional native extension that accelerates protocols and buffers.
