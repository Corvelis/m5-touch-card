import 'package:flutter/widgets.dart';
export 'card_catalog.dart';

String cardText(BuildContext context, String ja, String en) =>
    Localizations.localeOf(context).languageCode == 'en' ? en : ja;

const defaultCardLanguage = String.fromEnvironment(
  'TOUCH_CARD_DEFAULT_LANGUAGE',
  defaultValue: 'ja',
);
String normalizeCardLanguage(String code) => code == 'en' ? 'en' : 'ja';
