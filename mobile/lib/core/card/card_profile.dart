import 'dart:math';
import 'dart:convert';

class CardProfile {
  const CardProfile({
    required this.id,
    required this.revision,
    required this.name,
    this.account = '',
    this.email = '',
    this.url = '',
    this.comment = '',
  });
  final String id, name, account, email, url, comment;
  final int revision;

  factory CardProfile.create() {
    final random = Random.secure();
    final id = List.generate(
      16,
      (_) => random.nextInt(256).toRadixString(16).padLeft(2, '0'),
    ).join();
    return CardProfile(id: id, revision: 0, name: '');
  }
  // Empty names may exist only in a local draft, never in a publishable card.
  void validate({bool draft = false}) {
    if (!RegExp(r'^[a-f0-9]{32}$').hasMatch(id) ||
        revision < 0 ||
        revision > 0xffffffff) {
      throw const FormatException('名刺IDまたは更新番号が不正です');
    }
    if (!draft && name.trim().isEmpty) {
      throw const FormatException('名前を入力してください');
    }
    for (final entry in [
      (name, 80),
      (account, 80),
      (email, 254),
      (url, 512),
      (comment, 40),
    ]) {
      if (entry.$1.runes.length > entry.$2 ||
          RegExp(r'[\x00-\x1f\x7f]').hasMatch(entry.$1)) {
        throw const FormatException('文字数または使用できない文字を確認してください');
      }
    }
    if (url.isNotEmpty) {
      if (utf8.encode(url).length > 512) {
        throw const FormatException('QR用URLはUTF-8で512バイトまでです');
      }
      final parsed = Uri.tryParse(url);
      if (parsed == null ||
          !['https', 'http'].contains(parsed.scheme) ||
          parsed.host.isEmpty) {
        throw const FormatException('QR用URLには http または https のURLを入力してください');
      }
    }
  }

  CardProfile revise({
    required String name,
    required String account,
    required String email,
    required String url,
    required String comment,
  }) {
    final result = CardProfile(
      id: id,
      revision: revision + 1,
      name: name.trim(),
      account: account.trim(),
      email: email.trim(),
      url: url.trim(),
      comment: comment.trim(),
    );
    result.validate();
    return result;
  }

  CardProfile bumpRevision() => CardProfile(
    id: id,
    revision: revision + 1,
    name: name,
    account: account,
    email: email,
    url: url,
    comment: comment,
  )..validate(draft: true);
  Map<String, Object> toJson() => {
    'id': id,
    'revision': revision,
    'name': name,
    'account': account,
    'email': email,
    'url': url,
    'comment': comment,
  };
  factory CardProfile.fromJson(Map<String, dynamic> json) => CardProfile(
    id: json['id'] as String,
    revision: json['revision'] as int,
    name: json['name'] as String,
    account: json['account'] as String,
    email: json['email'] as String,
    url: json['url'] as String,
    comment: json['comment'] as String,
  )..validate(draft: true);
}
