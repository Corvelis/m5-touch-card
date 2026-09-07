import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:touch_card/core/card/workspace_store.dart';
import 'package:touch_card/core/card/card_profile.dart';
import 'package:touch_card/core/protocol/card_packet.dart';
import 'package:touch_card/features/transfer/card_transfer_screen.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  const methods = MethodChannel('io.github.corvelis.touch_card/methods');
  const events = MethodChannel('io.github.corvelis.touch_card/events');
  final calls = <MethodCall>[];
  late _MemoryStore store;
  late CardPacket packet;
  setUp(() async {
    store = _MemoryStore();
    packet = CardPacket.create(await store.load(), 1);
    await store.savePending(packet);
    calls.clear();
    final messenger =
        TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;
    messenger.setMockMethodCallHandler(methods, (call) async {
      calls.add(call);
      return null;
    });
    messenger.setMockMethodCallHandler(events, (_) async => null);
  });
  tearDown(() async {
    final messenger =
        TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;
    messenger.setMockMethodCallHandler(methods, null);
    messenger.setMockMethodCallHandler(events, null);
  });
  Future<void> event(
    WidgetTester tester,
    String phase, {
    String? error,
    int? id,
  }) async {
    await TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .handlePlatformMessage(
          events.name,
          const StandardMethodCodec().encodeSuccessEnvelope({
            'phase': phase,
            'transferId': id ?? (phase == 'clockSynced' ? 0 : packet.id),
            'bytesSent': packet.bytes.length,
            'totalBytes': packet.bytes.length,
            if (error != null) 'errorCode': error,
          }),
          (_) {},
        );
    await tester.pump();
  }

  testWidgets(
    '100 percent received is not saved; stored acknowledgement clears queue',
    (tester) async {
      await tester.pumpWidget(
        MaterialApp(
          locale: const Locale('ja'),
          supportedLocales: const [Locale('ja'), Locale('en')],
          localizationsDelegates: GlobalMaterialLocalizations.delegates,

          home: CardTransferScreen(store: store, packet: packet),
        ),
      );
      await tester.pump();
      final start = calls.firstWhere((call) => call.method == 'startTransfer');
      expect((start.arguments as Map)['bytes'], packet.bytes);
      expect((start.arguments as Map)['mode'], 1);
      expect((start.arguments as Map)['width'], 0);
      await event(tester, 'verifying');
      expect(find.text('本体に保存しました'), findsNothing);
      expect(store.state.pending, isNotNull);
      await event(tester, 'stored', id: packet.id == 1 ? 2 : 1);
      expect(find.text('本体に保存しました'), findsNothing);
      expect(store.state.pending, isNotNull);
      await event(tester, 'stored');
      expect(store.state.pending, isNull);
      await tester.pump();
      expect(find.text('本体に保存しました'), findsOneWidget);
      await tester.pumpWidget(const SizedBox());
    },
  );
  testWidgets('failure preserves pending and retries the exact same transfer', (
    tester,
  ) async {
    await tester.pumpWidget(
      MaterialApp(
        locale: const Locale('ja'),
        supportedLocales: const [Locale('ja'), Locale('en')],
        localizationsDelegates: GlobalMaterialLocalizations.delegates,

        home: CardTransferScreen(store: store, packet: packet),
      ),
    );
    await tester.pump();
    await event(tester, 'failed', error: 'NO_SPACE');
    expect(find.textContaining('保存容量が不足'), findsOneWidget);
    await tester.tap(find.text('同じ内容で再試行'));
    await tester.pump();
    final starts = calls
        .where((call) => call.method == 'startTransfer')
        .toList();
    expect(starts.length, 2);
    expect(starts[0].arguments, starts[1].arguments);
    await tester.pumpWidget(const SizedBox());
    expect(store.state.pending!.id, packet.id);
  });
  testWidgets(
    'clock-only sync never sends image/card and never clears queued card',
    (tester) async {
      await tester.pumpWidget(
        MaterialApp(
          locale: const Locale('ja'),
          supportedLocales: const [Locale('ja'), Locale('en')],
          localizationsDelegates: GlobalMaterialLocalizations.delegates,
          home: CardTransferScreen(store: store),
        ),
      );
      await tester.pump();
      expect(calls.where((call) => call.method == 'startTransfer'), isEmpty);
      expect(calls.where((call) => call.method == 'syncClock').length, 1);
      await event(tester, 'clockSynced');
      expect(find.text('本体に保存しました'), findsOneWidget);
      await tester.pumpWidget(const SizedBox());
      expect(store.state.pending!.id, packet.id);
    },
  );
  testWidgets(
    'English NFC UI forwards language without changing packet bytes',
    (tester) async {
      await tester.pumpWidget(
        MaterialApp(
          locale: const Locale('en'),
          home: CardTransferScreen(
            store: store,
            packet: packet,
            languageCode: 'en',
          ),
        ),
      );
      await tester.pump();
      expect(find.textContaining('Open Settings'), findsOneWidget);
      final start = calls.firstWhere((call) => call.method == 'startTransfer');
      expect((start.arguments as Map)['language'], 'en');
      expect((start.arguments as Map)['bytes'], packet.bytes);
      await event(tester, 'failed', error: 'NO_SPACE');
      expect(find.textContaining('Not enough space'), findsOneWidget);
      expect(store.state.pending!.id, packet.id);
      await tester.pumpWidget(const SizedBox());
    },
  );
}

// Disk durability is exercised by card_packet_test/card_workspace_test; keep
// widget scheduling independent from real filesystem callbacks.
class _MemoryStore extends WorkspaceStore {
  CardWorkspace state = CardWorkspace(
    profile: CardProfile.create().revise(
      name: 'Yuma',
      account: '',
      email: '',
      url: '',
      comment: '',
    ),
  );
  @override
  Future<CardWorkspace> load() async => state;
  @override
  Future<CardWorkspace> savePending(CardPacket packet) async =>
      state = CardWorkspace(
        profile: state.profile,
        images: state.images,
        pending: packet,
      );
  @override
  Future<CardWorkspace> clearPending(int id) async => state = CardWorkspace(
    profile: state.profile,
    images: state.images,
    pending: state.pending?.id == id ? null : state.pending,
  );
}
