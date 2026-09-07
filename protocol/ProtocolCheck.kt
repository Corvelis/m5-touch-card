package io.github.corvelis.touch_card

import java.io.File
import java.util.zip.CRC32

fun main(args: Array<String>) {
    val json = File(args.single()).readText()
    val fixtures = Regex("\"([a-z_]+)\"\\s*:\\s*\"([a-f0-9]+)\"")
        .findAll(json).associate { it.groupValues[1] to it.groupValues[2] }
    fun bytes(key: String) = fixtures.getValue(key).chunked(2).map { it.toInt(16).toByte() }.toByteArray()
    fun equal(key: String, actual: ByteArray) = check(bytes(key).contentEquals(actual)) { key }
    val payload = bytes("payload")
    val checksum = CRC32().apply { update(payload) }.value
    val transfer = PendingTransfer(payload, 1, 0, 0, checksum, 0x12345678L, 1800000000L, 540)
    equal("hello", PaperMonoProtocol.hello())
    equal("begin", PaperMonoProtocol.begin(transfer))
    equal("data", PaperMonoProtocol.data(transfer.transferId, 0, payload))
    equal("commit", PaperMonoProtocol.commit(transfer.transferId, checksum))
    equal("set_time", PaperMonoProtocol.setTime(1800000000L, 540))
    val hello = PaperMonoProtocol.parseResponse(bytes("hello_response"), PaperMonoCommand.HELLO)
    check(PaperMonoProtocol.parseHello(hello).maxImageBytes == 524288L)
    val extended = hello.copy(extra = hello.extra.copyOf().also { it[10] = 31 })
    check(PaperMonoProtocol.parseHello(extended).maxImageBytes == 524288L)
    val stored = PaperMonoProtocol.parseResponse(bytes("stored_response"), PaperMonoCommand.STATUS)
    check(stored.status == PaperMonoStatus.STORED && stored.nextExpectedOffset == payload.size.toLong())
    fun rejected(block: () -> Unit) { var caught = false; try { block() } catch (_: ProtocolException) { caught = true }; check(caught) }
    for (length in 0..12) rejected { PaperMonoProtocol.parseResponse(bytes("stored_response").copyOf(length), PaperMonoCommand.STATUS) }
    rejected { PaperMonoProtocol.parseResponse(bytes("stored_response").also { it[2] = 1 }, PaperMonoCommand.STATUS) }
    rejected { PaperMonoProtocol.parseHello(hello.copy(extra = hello.extra.copyOf().also { it[12] = 2 })) }
    println("Kotlin production encoder/parser: shared vectors PASS")
}
