import 'package:flutter/material.dart';
import 'card_strings.dart';

class CardLanguageMenu extends StatefulWidget {
  const CardLanguageMenu({
    super.key,
    required this.language,
    required this.onChanged,
    this.enabled = true,
  });
  final ValueNotifier<String> language;
  final Future<void> Function(String) onChanged;
  final bool enabled;
  @override
  State<CardLanguageMenu> createState() => _CardLanguageMenuState();
}

class _CardLanguageMenuState extends State<CardLanguageMenu> {
  bool saving = false;
  Future<void> _choose(String? code) async {
    if (code == null || saving || code == widget.language.value) return;
    setState(() => saving = true);
    try {
      await widget.onChanged(code);
    } on Object {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              cardText(context, '言語を保存できませんでした', 'Could not save the language'),
            ),
          ),
        );
      }
    } finally {
      if (mounted) setState(() => saving = false);
    }
  }

  @override
  Widget build(BuildContext context) => ValueListenableBuilder<String>(
    valueListenable: widget.language,
    builder: (context, code, _) => PopupMenuButton<String>(
      icon: const Icon(Icons.language),
      tooltip: '言語 / Language',
      position: PopupMenuPosition.under,
      enabled: widget.enabled && !saving,
      initialValue: code,
      onSelected: _choose,
      itemBuilder: (context) => [
        for (final option in {'ja': '日本語', 'en': 'English'}.entries)
          CheckedPopupMenuItem<String>(
            value: option.key,
            checked: code == option.key,
            child: Text(option.value),
          ),
      ],
    ),
  );
}
