# Ruby Fuzzing README

The Ruby Thrift implementation uses [Ruzzy](https://github.com/trailofbits/ruzzy) for fuzzing. Ruzzy is a coverage-guided fuzzer for pure Ruby code and Ruby C extensions.

We currently have several fuzz targets that test different aspects of the Thrift implementation:

* FuzzParseBinary -- fuzzes the deserialization of the Binary protocol
* FuzzParseBinaryAccelerated -- fuzzes the deserialization of the accelerated Binary protocol
* FuzzParseCompact -- fuzzes the deserialization of the Compact protocol
* FuzzParseCompactAccelerated -- fuzzes the deserialization of the accelerated Compact protocol
* FuzzRoundtripBinary -- fuzzes the roundtrip of the Binary protocol (i.e. serializes then deserializes and compares the result)
* FuzzRoundtripBinaryAccelerated -- fuzzes the roundtrip of the accelerated Binary protocol
* FuzzRoundtripCompact -- fuzzes the roundtrip of the Compact protocol
* FuzzRoundtripCompactAccelerated -- fuzzes the roundtrip of the accelerated Compact protocol

The fuzzers use Ruzzy's mutation engine to generate test cases. Each fuzzer implements the standard Ruzzy interface and uses common testing code from the fuzz test utilities in `fuzz_common.rb`.

For more information about Ruzzy and its options, see the [Ruzzy documentation](https://github.com/trailofbits/ruzzy).

You can also use the corpus generator from the Rust implementation to generate initial corpus files that can be used with these Ruby fuzzers, since the wire formats are identical between implementations.