/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

namespace rb ZeitwerkSpecNamespace

include "Referenced.thrift"

enum NotificationType {
  SUPPLIER_COMPANY
}

struct my_union_holder {
  1: i32 value
  2: Referenced.SomeEnum some_enum
  3: NotificationType notification_type
}

service zeitwerk_service {
  my_union_holder echo(1: my_union_holder holder)
}
