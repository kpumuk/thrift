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

$:.unshift File.dirname(__FILE__) + '/../../lib'
# $:.unshift File.dirname(__FILE__) + '/../../ext'
require 'thrift'
$:.unshift File.dirname(__FILE__) + "/gen-rb"
require 'fuzz_test_constants'

require 'coverage'
Coverage.start(branches: true)
require 'ruzzy'
# Ruzzy.enable_branch_coverage_hooks

def create_parser_fuzzer(protocol_factory_class, read_message_begin: false)
  lambda do |data|
    transport = Thrift::MemoryBufferTransport.new(data)
    protocol = protocol_factory_class.new.get_protocol(transport)
    protocol.read_message_begin if read_message_begin

    Fuzz::FuzzTest.new.read(protocol)
  rescue Thrift::ProtocolException, EOFError
    # We're looking for memory corruption, not Ruby exceptions
  rescue StandardError => e
    raise unless e.message =~ /Union fields are not set/
  end
end

def create_roundtrip_fuzzer(protocol_factory_class, read_message_begin: false)
  lambda do |data|
    transport = Thrift::MemoryBufferTransport.new(data)
    protocol = protocol_factory_class.new.get_protocol(transport)
    protocol.read_message_begin if read_message_begin
    obj = Fuzz::FuzzTest.new
    obj.read(protocol)

    serialized_data = +""
    transport2 = Thrift::MemoryBufferTransport.new(serialized_data)
    protocol2 = protocol_factory_class.new.get_protocol(transport2)
    obj.write(protocol2)

    transport3 = Thrift::MemoryBufferTransport.new(serialized_data)
    protocol3 = protocol_factory_class.new.get_protocol(transport3)
    protocol3.read_message_begin if read_message_begin
    deserialized_obj = Fuzz::FuzzTest.new
    deserialized_obj.read(protocol3)

    raise "Roundtrip mismatch" unless obj == deserialized_obj
  rescue Thrift::ProtocolException, EOFError, Encoding::UndefinedConversionError
    # We're looking for memory corruption, not Ruby exceptions
  rescue RuntimeError => e
    raise unless e.message =~ /don't know what ctype/ || e.message =~ /Too many fields for union/
  rescue RangeError => e
    raise unless e.message =~ /bignum too big to convert into 'long'/
  rescue StandardError => e
    raise unless e.message =~ /Union fields are not set/
  end
end
