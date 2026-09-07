import 'dart:typed_data';
import 'package:crop_your_image/crop_your_image.dart';
import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'l10n/card_strings.dart';
import 'l10n/card_language_menu.dart';
import 'l10n/language_preference_store.dart';
import 'package:image_picker/image_picker.dart';
import 'core/card/workspace_store.dart';
import 'core/image/card_image_processor.dart';
import 'features/image_editor/image_source_service.dart';
import 'core/protocol/card_packet.dart';
import 'features/transfer/card_transfer_screen.dart';

class TouchCardApp extends StatefulWidget {
  const TouchCardApp({
    super.key,
    this.store,
    this.languageStore = const LanguagePreferenceStore(),
  });
  final WorkspaceStore? store;
  final LanguagePreferenceStore languageStore;
  @override
  State<TouchCardApp> createState() => _TouchCardAppState();
}

class _TouchCardAppState extends State<TouchCardApp> {
  final language = ValueNotifier(normalizeCardLanguage(defaultCardLanguage));
  late final store = widget.store ?? WorkspaceStore();
  bool chosen = false;
  @override
  void initState() {
    super.initState();
    _restoreLanguage();
  }

  Future<void> _restoreLanguage() async {
    final code = await widget.languageStore.loadCode(fallback: language.value);
    if (mounted && !chosen) language.value = normalizeCardLanguage(code);
  }

  Future<void> _setLanguage(String code) async {
    chosen = true;
    await widget.languageStore.saveCode(code);
    if (mounted) language.value = code;
  }

  @override
  void dispose() {
    language.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => ValueListenableBuilder<String>(
    valueListenable: language,
    builder: (context, code, _) => MaterialApp(
      title: 'Touch Card',
      locale: Locale(code),
      supportedLocales: [Locale('ja'), Locale('en')],
      localizationsDelegates: GlobalMaterialLocalizations.delegates,
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        scaffoldBackgroundColor: const Color(0xfff5f4f0),
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xff263e38)),
        inputDecorationTheme: InputDecorationTheme(
          border: OutlineInputBorder(),
        ),
      ),
      home: WorkspaceScreen(
        store: store,
        language: language,
        onLanguageChanged: _setLanguage,
      ),
    ),
  );
}

class WorkspaceScreen extends StatefulWidget {
  const WorkspaceScreen({
    super.key,
    required this.store,
    this.language,
    this.onLanguageChanged,
  });
  final WorkspaceStore store;
  final ValueNotifier<String>? language;
  final Future<void> Function(String)? onLanguageChanged;
  @override
  State<WorkspaceScreen> createState() => _WorkspaceScreenState();
}

class _WorkspaceScreenState extends State<WorkspaceScreen> {
  final name = TextEditingController(),
      account = TextEditingController(),
      email = TextEditingController(),
      url = TextEditingController(),
      comment = TextEditingController();
  CardWorkspace? workspace;
  String? error;
  bool busy = true, dirty = false;
  int tab = 0;
  ImageSlot imageSlot = ImageSlot.dashboard;
  CardDevice device = CardDevice.paperMono;
  @override
  void initState() {
    super.initState();
    _load();
  }

  @override
  void dispose() {
    for (final controller in [name, account, email, url, comment]) {
      controller.dispose();
    }
    super.dispose();
  }

  void _fields(CardWorkspace state) {
    final profile = state.profile;
    name.text = profile.name;
    account.text = profile.account;
    email.text = profile.email;
    url.text = profile.url;
    comment.text = profile.comment;
  }

  Future<void> _load() async {
    try {
      final state = await widget.store.load();
      if (!mounted) return;
      _fields(state);
      setState(() {
        workspace = state;
        device = state.images[imageSlot]?.device ?? CardDevice.paperMono;
        busy = false;
        error = null;
      });
    } on Object catch (e) {
      if (mounted) {
        setState(() {
          busy = false;
          error = '$e';
        });
      }
    }
  }

  void _message(String message) {
    if (mounted) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text(cardNotice(context, message))));
    }
  }

  Future<void> _save() async {
    setState(() => busy = true);
    try {
      final state = await widget.store.saveProfile(
        name: name.text,
        account: account.text,
        email: email.text,
        url: url.text,
        comment: comment.text,
      );
      if (!mounted) return;
      _fields(state);
      setState(() {
        workspace = state;
        dirty = false;
      });
      _message(cardLabel(context, 'このスマホに保存しました'));
    } on Object catch (e) {
      _message('$e');
    } finally {
      if (mounted) setState(() => busy = false);
    }
  }

  Future<void> _image(ImageSlot slot, {bool recrop = false}) async {
    setState(() => busy = true);
    try {
      final source = recrop
          ? workspace!.images[slot]?.source
          : await ImageSourceService().pick(ImageSource.gallery);
      if (source == null || !mounted) return;
      final selectedDevice = device;
      final (width, height) = CardImageProcessor.dimensions(
        slot,
        selectedDevice,
      );
      final cropped = await Navigator.of(context).push<Uint8List>(
        MaterialPageRoute(
          builder: (_) =>
              ImageCropScreen(bytes: source, aspectRatio: width / height),
        ),
      );
      if (cropped == null || !mounted) return;
      final image = await CardImageProcessor().prepare(
        source,
        cropped,
        slot,
        selectedDevice,
      );
      final state = await widget.store.saveImage(slot, image);
      if (!mounted) return;
      setState(() => workspace = state);
      _message(cardLabel(context, 'この用途の画像だけを保存しました'));
    } on Object catch (e) {
      _message('$e');
    } finally {
      if (mounted) setState(() => busy = false);
    }
  }

  void _selectImageSlot(ImageSlot slot) {
    if (busy) return;
    setState(() {
      imageSlot = slot;
      device = workspace!.images[slot]?.device ?? device;
    });
  }

  Future<void> _selectImageDevice(CardDevice? target) async {
    if (target == null || busy || target == device) return;
    final slot = imageSlot;
    final original = workspace!.images[slot];
    if (original == null) {
      setState(() => device = target);
      return;
    }
    setState(() => busy = true);
    try {
      // Always work from the retained original, never repeatedly recompress a
      // smaller device JPEG. Each purpose has its own independent source.
      final image = await CardImageProcessor().prepare(
        original.source,
        original.source,
        slot,
        target,
      );
      final state = await widget.store.saveImage(slot, image);
      if (!mounted) return;
      setState(() {
        workspace = state;
        device = target;
      });
    } on Object catch (e) {
      // Keep the previous target/image usable if conversion or saving fails.
      _message('$e');
    } finally {
      if (mounted) setState(() => busy = false);
    }
  }

  Future<void> _transfer(CardPacket? packet) async {
    await Navigator.of(context).push<void>(
      MaterialPageRoute(
        builder: (_) => CardTransferScreen(
          store: widget.store,
          packet: packet,
          languageCode: Localizations.localeOf(context).languageCode,
        ),
      ),
    );
    try {
      final state = await widget.store.load();
      if (mounted) setState(() => workspace = state);
    } on Object catch (e) {
      _message('$e');
    }
  }

  Future<void> _send(int target, {bool textOnly = false}) async {
    if (target <= 2 && dirty) {
      await _save();
      if (dirty || !mounted) return;
    }
    try {
      final packet = CardPacket.create(workspace!, target, textOnly: textOnly);
      if (workspace!.pending != null) {
        final replace = await showDialog<bool>(
          context: context,
          builder: (context) => AlertDialog(
            title: Text(cardLabel(context, '送信待ちを置き換えますか？')),
            content: Text(
              cardText(
                context,
                '${workspace!.pending!.label}の未完了送信が残っています。',
                'A transfer of ${cardLabel(context, workspace!.pending!.label)} is still pending.',
              ),
            ),
            actions: [
              TextButton(
                onPressed: () => Navigator.pop(context, false),
                child: Text(cardLabel(context, 'キャンセル')),
              ),
              TextButton(
                onPressed: () => Navigator.pop(context, true),
                child: Text(cardLabel(context, '置き換える')),
              ),
            ],
          ),
        );
        if (replace != true || !mounted) return;
      }
      final state = await widget.store.savePending(packet);
      if (!mounted) return;
      setState(() => workspace = state);
      await _transfer(packet);
    } on Object catch (e) {
      _message('$e');
    }
  }

  Future<void> _resetAvatar() async {
    try {
      final state = await widget.store.resetAvatar();
      if (!mounted) return;
      setState(() => workspace = state);
      _message(cardLabel(context, 'スマホのアイコンを初期画像に戻しました。NFC更新で本体へ反映します。'));
    } on Object catch (e) {
      _message('$e');
    }
  }

  Future<void> _optimizeExchange() async {
    final avatar = workspace?.images[ImageSlot.avatar];
    if (avatar == null) return;
    setState(() => busy = true);
    try {
      final image = await CardImageProcessor().prepareExchange(avatar);
      final state = await widget.store.saveImage(ImageSlot.avatar, image);
      if (!mounted) return;
      setState(() => workspace = state);
      _message(cardLabel(context, '交換用アイコンを保存しました。NFCで名刺を更新すると本体に反映されます。'));
    } on Object catch (e) {
      _message('$e');
    } finally {
      if (mounted) setState(() => busy = false);
    }
  }

  Widget _exchangePreview() {
    final avatar = workspace!.images[ImageSlot.avatar];
    if (avatar == null) return const SizedBox.shrink();
    final bytes = avatar.exchangeJpeg ?? avatar.jpeg;
    final info = avatar.exchangeInfo;
    final needsResize =
        avatar.exchangeJpeg == null ||
        info.width != CardImageProcessor.exchangeSide ||
        info.height != CardImageProcessor.exchangeSide;
    final withinBudget = bytes.length <= CardImageProcessor.exchangeBudgetBytes;
    return ExpansionTile(
      tilePadding: EdgeInsets.zero,
      title: Text(cardLabel(context, '交換用アイコン')),
      subtitle: Text(
        '${(bytes.length / 1024).toStringAsFixed(1)} KB · ${info.width} × ${info.height}',
      ),
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            for (final item in [
              (cardLabel(context, '本体表示'), avatar.jpeg),
              (cardLabel(context, '相手に渡す画像'), bytes),
            ])
              Column(
                children: [
                  ClipOval(
                    child: Image.memory(
                      item.$2,
                      width: 112,
                      height: 112,
                      fit: BoxFit.cover,
                    ),
                  ),
                  const SizedBox(height: 8),
                  Text(item.$1),
                ],
              ),
          ],
        ),
        const SizedBox(height: 16),
        Text(
          needsResize
              ? cardLabel(context, '本体表示はそのままに、交換用だけ160 × 160に縮小できます。')
              : withinBudget
              ? cardLabel(context, '交換用は160 × 160・6 KB以内です。')
              : cardLabel(context, '交換用は160 × 160です。画質を保つため6 KBを超えています。'),
        ),
        if (needsResize)
          TextButton(
            onPressed: busy ? null : _optimizeExchange,
            child: Text(cardLabel(context, '交換用を160pxにする')),
          ),
        const SizedBox(height: 12),
      ],
    );
  }

  Widget _photo(ImageSlot slot, {double? height}) {
    final image = workspace?.images[slot];
    final child = image == null
        ? Image.asset('assets/default.jpg', height: height, fit: BoxFit.cover)
        : Image.memory(image.jpeg, height: height, fit: BoxFit.cover);
    if (slot == ImageSlot.avatar) return ClipOval(child: child);
    return ClipRRect(borderRadius: BorderRadius.circular(4), child: child);
  }

  Widget _field(
    TextEditingController controller,
    String label,
    int max, {
    TextInputType? keyboard,
  }) => Padding(
    padding: const EdgeInsets.only(bottom: 16),
    child: TextField(
      controller: controller,
      enabled: !busy,
      keyboardType: keyboard,
      maxLength: max,
      onChanged: (_) => setState(() => dirty = true),
      decoration: InputDecoration(labelText: label, counterText: ''),
    ),
  );
  Widget _card() => Column(
    crossAxisAlignment: CrossAxisAlignment.stretch,
    children: [
      Container(
        padding: const EdgeInsets.all(24),
        color: Colors.white,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Expanded(
                  child: Text(
                    name.text.trim().isEmpty
                        ? cardLabel(context, 'あなたの名前')
                        : name.text,
                    style: TextStyle(
                      fontSize: 30,
                      fontWeight: FontWeight.w700,
                      height: 1.15,
                    ),
                  ),
                ),
                const SizedBox(width: 16),
                SizedBox(
                  width: 68,
                  height: 68,
                  child: _photo(ImageSlot.avatar),
                ),
              ],
            ),
            if (account.text.isNotEmpty)
              Padding(
                padding: const EdgeInsets.only(top: 12),
                child: Text(account.text),
              ),
            const Padding(
              padding: EdgeInsets.symmetric(vertical: 16),
              child: Divider(),
            ),
            if (email.text.isNotEmpty) Text(email.text),
            if (comment.text.isNotEmpty)
              Padding(
                padding: const EdgeInsets.only(top: 12),
                child: Text(comment.text),
              ),
          ],
        ),
      ),
      TextButton.icon(
        onPressed: busy ? null : () => _image(ImageSlot.avatar),
        icon: const Icon(Icons.add_photo_alternate_outlined),
        label: Text(cardLabel(context, 'アイコンを変更')),
      ),
      if (workspace!.images.containsKey(ImageSlot.avatar))
        TextButton(
          onPressed: busy ? null : () => _image(ImageSlot.avatar, recrop: true),
          child: Text(cardLabel(context, 'アイコンを切り抜き直す')),
        ),
      const SizedBox(height: 16),
      _exchangePreview(),
      _field(name, cardLabel(context, '名前'), 80),
      ExpansionTile(
        title: Text(cardLabel(context, '追加情報（任意）')),
        tilePadding: EdgeInsets.zero,
        children: [
          _field(account, cardLabel(context, 'アカウント名'), 80),
          _field(
            email,
            cardLabel(context, 'メールアドレス'),
            254,
            keyboard: TextInputType.emailAddress,
          ),
          _field(
            url,
            cardLabel(context, 'QR用URL'),
            512,
            keyboard: TextInputType.url,
          ),
          _field(comment, cardLabel(context, '一言コメント（40文字まで）'), 40),
        ],
      ),
      const SizedBox(height: 20),
      FilledButton(
        onPressed: busy ? null : _save,
        child: Text(
          dirty ? cardLabel(context, '変更を保存') : cardLabel(context, 'スマホに保存'),
        ),
      ),
      const SizedBox(height: 8),
      Row(
        children: [
          Expanded(
            child: FilledButton.icon(
              onPressed: busy ? null : () => _send(1),
              icon: const Icon(Icons.nfc),
              label: Text(cardLabel(context, 'NFCで名刺を更新')),
            ),
          ),
          PopupMenuButton<int>(
            tooltip: cardLabel(context, '更新する内容'),
            enabled: !busy,
            onSelected: (value) {
              if (value == 1) {
                _send(1, textOnly: true);
              } else if (value == 2) {
                _send(2);
              } else {
                _resetAvatar();
              }
            },
            itemBuilder: (_) => [
              PopupMenuItem(
                value: 1,
                child: Text(cardLabel(context, '内容だけ更新')),
              ),
              PopupMenuItem(
                value: 2,
                child: Text(cardLabel(context, 'アイコンだけ更新')),
              ),
              PopupMenuItem(
                value: 3,
                child: Text(cardLabel(context, 'アイコンを初期画像に戻す')),
              ),
            ],
          ),
        ],
      ),
    ],
  );
  Widget _images() {
    final image = workspace!.images[imageSlot];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        SegmentedButton<ImageSlot>(
          segments: [
            ButtonSegment(
              value: ImageSlot.dashboard,
              label: Text(cardLabel(context, '日時・カレンダー')),
            ),
            ButtonSegment(
              value: ImageSlot.fullscreen,
              label: Text(cardLabel(context, '全画面')),
            ),
          ],
          selected: {imageSlot},
          onSelectionChanged: busy
              ? null
              : (value) => _selectImageSlot(value.single),
        ),
        const SizedBox(height: 16),
        DropdownButtonFormField<CardDevice>(
          key: ValueKey('image-device-${imageSlot.name}-${device.name}-$busy'),
          value: device,
          decoration: InputDecoration(labelText: cardLabel(context, '画像を送る端末')),
          items: [
            DropdownMenuItem(
              value: CardDevice.paperMono,
              child: Text('Paper Mono'),
            ),
            DropdownMenuItem(
              value: CardDevice.stackchan,
              child: Text(cardLabel(context, 'ｽﾀｯｸﾁｬﾝ')),
            ),
          ],
          onChanged: busy ? null : _selectImageDevice,
        ),
        const SizedBox(height: 20),
        SizedBox(
          height: 300,
          child: Center(
            child: AspectRatio(
              aspectRatio: image == null
                  ? CardImageProcessor.dimensions(imageSlot, device).$1 /
                        CardImageProcessor.dimensions(imageSlot, device).$2
                  : image.width / image.height,
              child: _photo(imageSlot),
            ),
          ),
        ),
        const SizedBox(height: 12),
        Text(
          image == null
              ? cardLabel(context, '初期画像')
              : '${cardText(context, '保存済み', 'Saved')}: ${image.device == CardDevice.paperMono ? 'Paper Mono' : cardLabel(context, 'ｽﾀｯｸﾁｬﾝ')} · ${image.width} × ${image.height}',
          textAlign: TextAlign.center,
        ),
        const SizedBox(height: 20),
        FilledButton.icon(
          onPressed: busy ? null : () => _image(imageSlot),
          icon: const Icon(Icons.photo_outlined),
          label: Text(cardLabel(context, '画像を選んで切り抜く')),
        ),
        if (image != null)
          TextButton(
            onPressed: busy ? null : () => _image(imageSlot, recrop: true),
            child: Text(cardLabel(context, '保存した元画像から切り抜き直す')),
          ),
        const SizedBox(height: 12),
        FilledButton.icon(
          onPressed: busy || image == null
              ? null
              : () => _send(imageSlot == ImageSlot.dashboard ? 3 : 4),
          icon: const Icon(Icons.nfc),
          label: Text(cardLabel(context, 'この画像をNFCで送る')),
        ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) => Scaffold(
    appBar: AppBar(
      title: Text(
        tab == 0 ? cardLabel(context, '自分の名刺') : cardLabel(context, 'ホーム画像'),
      ),
      actions: [
        if (widget.language != null && widget.onLanguageChanged != null)
          CardLanguageMenu(
            language: widget.language!,
            onChanged: widget.onLanguageChanged!,
            enabled: !busy,
          ),
        IconButton(
          icon: const Icon(Icons.schedule),
          tooltip: cardLabel(context, '日時を合わせる'),
          onPressed: busy ? null : () => _transfer(null),
        ),
        IconButton(
          icon: const Icon(Icons.info_outline),
          tooltip: cardText(context, 'ライセンス', 'Licenses'),
          onPressed: () =>
              showLicensePage(context: context, applicationName: 'Touch Card'),
        ),
      ],
    ),
    body: SafeArea(
      child: workspace == null
          ? Center(
              child: error == null
                  ? const CircularProgressIndicator()
                  : Padding(
                      padding: const EdgeInsets.all(24),
                      child: Text(cardNotice(context, error!)),
                    ),
            )
          : Center(
              child: ConstrainedBox(
                constraints: const BoxConstraints(maxWidth: 560),
                child: AbsorbPointer(
                  absorbing: busy,
                  child: ListView(
                    padding: const EdgeInsets.all(24),
                    children: [
                      if (busy) const LinearProgressIndicator(),
                      if (workspace!.pending != null)
                        ListTile(
                          leading: const Icon(Icons.upload_outlined),
                          title: Text(
                            '${cardText(context, '送信待ち', 'Pending')}: ${cardLabel(context, workspace!.pending!.label)}',
                          ),
                          subtitle: Text(cardLabel(context, 'タップして同じ内容で再開')),
                          onTap: busy
                              ? null
                              : () => _transfer(workspace!.pending),
                        ),
                      tab == 0 ? _card() : _images(),
                    ],
                  ),
                ),
              ),
            ),
    ),
    bottomNavigationBar: NavigationBar(
      selectedIndex: tab,
      onDestinationSelected: busy
          ? null
          : (value) => setState(() => tab = value),
      destinations: [
        NavigationDestination(
          icon: Icon(Icons.badge_outlined),
          label: cardLabel(context, '名刺'),
        ),
        NavigationDestination(
          icon: Icon(Icons.image_outlined),
          label: cardLabel(context, '画像'),
        ),
      ],
    ),
  );
}

class ImageCropScreen extends StatefulWidget {
  const ImageCropScreen({
    super.key,
    required this.bytes,
    required this.aspectRatio,
  });
  final Uint8List bytes;
  final double aspectRatio;
  @override
  State<ImageCropScreen> createState() => _ImageCropScreenState();
}

class _ImageCropScreenState extends State<ImageCropScreen> {
  final controller = CropController();
  bool ready = false, cropping = false;
  @override
  Widget build(BuildContext context) => Scaffold(
    appBar: AppBar(
      title: Text(cardLabel(context, '切り抜き')),
      actions: [
        TextButton(
          onPressed: ready && !cropping
              ? () {
                  setState(() => cropping = true);
                  controller.crop();
                }
              : null,
          child: Text(cardLabel(context, 'この範囲を保存')),
        ),
      ],
    ),
    body: Crop(
      image: widget.bytes,
      controller: controller,
      aspectRatio: widget.aspectRatio,
      onStatusChanged: (status) {
        if (mounted) setState(() => ready = status == CropStatus.ready);
      },
      onCropped: (result) {
        if (!mounted) return;
        if (result is CropSuccess) {
          Navigator.of(context).pop(result.croppedImage);
        } else {
          setState(() => cropping = false);
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text(cardLabel(context, '切り抜きに失敗しました'))),
          );
        }
      },
    ),
  );
}
