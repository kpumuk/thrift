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
require 'zeitwerk'

describe 'zeitwerk generation' do
  around do |example|
    loader = Zeitwerk::Loader.new
    setup = false
    loader.push_dir(File.expand_path('../gen-rb', __FILE__))
    loader.enable_reloading
    loader.setup
    setup = true
    example.run
  ensure
    loader.unload if setup
    loader.unregister
  end

  it 'generated strict file layout' do
    prefix = File.expand_path('../gen-rb', __FILE__)
    [
      'zeitwerk_spec_namespace/my_union_holder.rb',
      'zeitwerk_spec_namespace/notification_type.rb',
      'zeitwerk_spec_namespace/zeitwerk_service.rb',
      'zeitwerk_spec_namespace/zeitwerk_service/client.rb',
      'zeitwerk_spec_namespace/zeitwerk_service/processor.rb',
      'zeitwerk_spec_namespace/zeitwerk_service/echo_args.rb',
      'zeitwerk_spec_namespace/zeitwerk_service/echo_result.rb',
      'other_namespace/some_enum.rb',
      'base/base_service/client.rb',
      'extended/extended_service/client.rb'
    ].each do |name|
      expect(File.exist?(File.join(prefix, name))).to be_truthy
    end
  end

  it 'does not generate legacy aggregate files' do
    prefix = File.expand_path('../gen-rb', __FILE__)
    [
      'zeitwerk_spec_namespace/thrift_zeitwerk_spec_types.rb',
      'zeitwerk_spec_namespace/thrift_zeitwerk_spec_constants.rb'
    ].each do |name|
      expect(File.exist?(File.join(prefix, name))).not_to be_truthy
    end
  end

  it 'normalizes underscored names to Zeitwerk-compatible constants' do
    expect($LOADED_FEATURES.any? { |path| path.end_with?('/zeitwerk_spec_namespace/my_union_holder.rb') }).not_to be_truthy
    expect($LOADED_FEATURES.any? { |path| path.end_with?('/zeitwerk_spec_namespace/zeitwerk_service.rb') }).not_to be_truthy

    expect(ZeitwerkSpecNamespace::MyUnionHolder.name).to eq('ZeitwerkSpecNamespace::MyUnionHolder')
    expect(ZeitwerkSpecNamespace::ZeitwerkService::Client.name).to eq('ZeitwerkSpecNamespace::ZeitwerkService::Client')
  end

  it 'preserves underscores in enum value constants' do
    expect(ZeitwerkSpecNamespace::NotificationType::SUPPLIER_COMPANY).to eq(0)
    expect(defined?(ZeitwerkSpecNamespace::NotificationType::SUPPLIERCOMPANY)).to be_nil
  end

  it 'loads local enum lazily through validation path' do
    before = $LOADED_FEATURES.dup
    holder = ZeitwerkSpecNamespace::MyUnionHolder.new(:value => 7)
    loaded_after_struct = ($LOADED_FEATURES - before).grep(/gen-rb/)

    expect(loaded_after_struct.any? { |path| path.end_with?('/zeitwerk_spec_namespace/notification_type.rb') }).not_to be_truthy

    holder.notification_type = 0
    holder.validate
    loaded_after_validate = ($LOADED_FEATURES - before).grep(/gen-rb/)
    expect(loaded_after_validate.any? { |path| path.end_with?('/zeitwerk_spec_namespace/notification_type.rb') }).to be_truthy
  end
end
