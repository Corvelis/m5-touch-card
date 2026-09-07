import '../../l10n/card_strings.dart';
import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../core/card/workspace_store.dart';
import '../../core/protocol/card_packet.dart';
import 'nfc_transfer_bridge.dart';

class CardTransferScreen extends StatefulWidget {
  const CardTransferScreen({
    super.key,
    required this.store,
    this.packet,
    this.languageCode = 'ja',
  });
  final WorkspaceStore store;
  final String languageCode;
  final CardPacket? packet; // null = clock only
  @override
  State<CardTransferScreen> createState() => _CardTransferScreenState();
}

class _CardTransferScreenState extends State<CardTransferScreen> {
  static const methods = MethodChannel('io.github.corvelis.touch_card/methods');
  StreamSubscription<NfcTransferEvent>? subscription;
  String status = '本体の「設定 → スマホから更新」を開いてタッチしてください';
  double? progress;
  bool completed = false, finishing = false, failed = false;
  Timer? timeout;
  @override
  void initState() {
    super.initState();
    subscription = const NfcTransferBridge().events.listen(
      _event,
      onError: (Object error) => _error('$error'),
    );
    _start();
  }

  Future<void> _start() async {
    if (mounted) {
      setState(() {
        failed = completed = false;
        progress = null;
      });
    }
    try {
      await methods.invokeMethod<void>('cancelTransfer');
      final packet = widget.packet;
      Future<void> begin() async {
        if (packet == null) {
          final now = DateTime.now();
          await methods.invokeMethod<void>('syncClock', {
            'unixTimeSeconds': now.millisecondsSinceEpoch ~/ 1000,
            'utcOffsetMinutes': now.timeZoneOffset.inMinutes,
            'language': widget.languageCode,
          });
        } else {
          await methods.invokeMethod<void>('startTransfer', {
            'bytes': packet.bytes,
            'mode': packet.target,
            'width': 0,
            'height': 0,
            'crc32': packet.checksum,
            'transferId': packet.id,
            'language': widget.languageCode,
          });
        }
      }

      // Core NFC invalidation / Android's old worker can finish asynchronously.
      // Wait for that session to release before creating a new one.
      for (var attempt = 0; ; attempt++) {
        if (!mounted) return;
        try {
          await begin();
          break;
        } on PlatformException catch (error) {
          if (error.code != 'TRANSFER_IN_PROGRESS' || attempt >= 20) rethrow;
          await Future<void>.delayed(const Duration(milliseconds: 150));
        }
      }
      timeout?.cancel();
      timeout = Timer(
        const Duration(seconds: 120),
        () => _error(cardLabel(context, '通信時間を超えました。再試行できます。')),
      );
    } on PlatformException catch (e) {
      if (mounted) _error(cardNfcError(context, e.code, fallback: e.message));
    } on Object catch (e) {
      _error('$e');
    }
  }

  void _error(String error) {
    timeout?.cancel();
    if (!mounted || completed) return;
    setState(() {
      failed = true;
      status = error;
    });
  }

  Future<void> _event(NfcTransferEvent event) async {
    if (!mounted || completed || finishing) return;
    final success = widget.packet == null
        ? event.phase == NfcTransferPhase.clockSynced
        : [
            NfcTransferPhase.stored,
            NfcTransferPhase.completed,
            NfcTransferPhase.displaying,
          ].contains(event.phase);
    if (success) {
      if (event.transferId != (widget.packet?.id ?? 0)) return;
      finishing = true;
      timeout?.cancel();
      try {
        if (widget.packet != null) {
          await widget.store.clearPending(widget.packet!.id);
        }
        if (mounted) {
          setState(() {
            completed = true;
            status = cardLabel(context, '本体に保存しました');
            progress = 1;
          });
        }
      } on Object {
        if (mounted) {
          setState(() {
            completed = true;
            status = cardLabel(context, '本体に保存しました。スマホの送信履歴の更新に失敗しました。');
          });
        }
      }
      return;
    }
    if (event.phase == NfcTransferPhase.failed ||
        event.phase == NfcTransferPhase.recoverableError) {
      final errors = {
        'NO_SPACE': cardLabel(context, '保存容量が不足しています。名刺を削除するかSDを確認してください。'),
        'noSpace': cardLabel(context, '保存容量が不足しています。名刺を削除するかSDを確認してください。'),
        'MEDIA_LOST': cardLabel(context, '保存先のSDが取り外されました。'),
        'mediaLost': cardLabel(context, '保存先のSDが取り外されました。'),
        'CONFLICT': cardLabel(context, '名刺の更新世代が競合しています。内容を保存し直してください。'),
        'conflict': cardLabel(context, '名刺の更新世代が競合しています。内容を保存し直してください。'),
        'WRONG_SESSION': cardLabel(context, '本体の「設定 → スマホから更新」を開いてください。'),
        'wrongSession': cardLabel(context, '本体の「設定 → スマホから更新」を開いてください。'),
        'INVALID_JPEG': cardLabel(context, '名刺または画像の形式を確認してください。'),
        'invalidJpeg': cardLabel(context, '名刺または画像の形式を確認してください。'),
        'INTERNAL_ERROR': cardLabel(context, '本体への保存に失敗しました。ストレージを確認してください。'),
        'internalError': cardLabel(context, '本体への保存に失敗しました。ストレージを確認してください。'),
      };
      _error(
        errors[event.errorCode] ??
            cardNfcError(context, event.errorCode, fallback: event.message),
      );
      return;
    }
    if (event.totalBytes > 0) {
      progress = (event.bytesSent / event.totalBytes).clamp(0, 1);
    }
    setState(() {
      status = switch (event.phase) {
        NfcTransferPhase.receiving => cardLabel(context, '送信しています'),
        NfcTransferPhase.verifying => cardLabel(context, '本体への保存を確認しています'),
        NfcTransferPhase.clockSyncing => cardLabel(context, '日時を合わせています'),
        _ => cardLabel(context, '本体の「設定 → スマホから更新」を開いてタッチしてください'),
      };
    });
  }

  Future<void> _close() async {
    try {
      await methods.invokeMethod<void>('cancelTransfer');
    } on Object {
      /* leave pending intact */
    }
    if (mounted) Navigator.of(context).pop();
  }

  @override
  void dispose() {
    timeout?.cancel();
    subscription?.cancel();
    methods.invokeMethod<void>('cancelTransfer').catchError((Object _) {});
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => PopScope(
    canPop: false,
    onPopInvokedWithResult: (didPop, _) {
      if (!didPop) _close();
    },
    child: Scaffold(
      appBar: AppBar(
        title: Text(
          (widget.packet == null
                  ? null
                  : cardLabel(context, widget.packet!.label)) ??
              cardLabel(context, '日時を合わせる'),
        ),
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(28),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Icon(
                completed ? Icons.check_circle_outline : Icons.nfc,
                size: 72,
              ),
              const SizedBox(height: 32),
              Text(cardNotice(context, status), textAlign: TextAlign.center),
              const SizedBox(height: 24),
              if (!completed && !failed)
                LinearProgressIndicator(value: progress),
              if (failed)
                FilledButton(
                  onPressed: _start,
                  child: Text(cardLabel(context, '同じ内容で再試行')),
                ),
              const SizedBox(height: 24),
              TextButton(
                onPressed: _close,
                child: Text(
                  completed
                      ? cardLabel(context, '閉じる')
                      : widget.packet == null
                      ? cardLabel(context, '中止する')
                      : cardLabel(context, '閉じる（送信待ちは保持）'),
                ),
              ),
            ],
          ),
        ),
      ),
    ),
  );
}
