func verifyProtocol() throws {
  let json = try Data(contentsOf: URL(fileURLWithPath: CommandLine.arguments[1]))
  let fixtures = try JSONSerialization.jsonObject(with: json) as! [String: String]
  func bytes(_ key: String) -> Data {
    let hex = Array(fixtures[key]!)
    return Data(stride(from: 0, to: hex.count, by: 2).map { UInt8(String(hex[$0...($0 + 1)]), radix: 16)! })
  }
  func equal(_ key: String, _ actual: Data) { precondition(bytes(key) == actual, key) }
  let payload = bytes("payload")
  let checksum = bytes("commit").readUInt32LittleEndian(at: 8)
  let transfer = PendingTransfer(bytes: payload, mode: 1, width: 0, height: 0,
    crc32: checksum, transferId: 0x12345678, unixTimeSeconds: 1800000000,
    utcOffsetMinutes: 540, clockOnly: false)
  equal("hello", PaperMonoProtocol.hello())
  equal("begin", PaperMonoProtocol.begin(transfer))
  equal("data", PaperMonoProtocol.data(transferId: transfer.transferId, offset: 0, payload: payload))
  equal("commit", PaperMonoProtocol.commit(transferId: transfer.transferId, crc32: checksum))
  equal("set_time", PaperMonoProtocol.setTime(unixTimeSeconds: 1800000000, utcOffsetMinutes: 540))
  let hello = try PaperMonoProtocol.parseResponse(bytes("hello_response"), expected: .hello)
  let capabilities = try PaperMonoProtocol.parseHello(hello)
  precondition(capabilities.maxImageBytes == 524288)
  var extendedHello = bytes("hello_response"); extendedHello[23] = 31
  let extended = try PaperMonoProtocol.parseHello(PaperMonoProtocol.parseResponse(extendedHello, expected: .hello))
  precondition(extended.maxImageBytes == 524288 && extended.supportsTypedUpdates)
  let stored = try PaperMonoProtocol.parseResponse(bytes("stored_response"), expected: .status)
  precondition(stored.status == .stored && stored.nextExpectedOffset == payload.count)
  func rejected(_ block: () throws -> Void) {
    var caught = false
    do { try block() } catch { caught = true }
    precondition(caught)
  }
  for length in 0...12 {
    rejected { _ = try PaperMonoProtocol.parseResponse(bytes("stored_response").prefix(length), expected: .status) }
  }
  var old = bytes("stored_response"); old[2] = 1
  rejected { _ = try PaperMonoProtocol.parseResponse(old, expected: .status) }
  var wrongSession = bytes("hello_response"); wrongSession[25] = 2
  rejected { _ = try PaperMonoProtocol.parseHello(PaperMonoProtocol.parseResponse(wrongSession, expected: .hello)) }
  print("Swift production encoder/parser: shared vectors PASS")
}
try verifyProtocol()
