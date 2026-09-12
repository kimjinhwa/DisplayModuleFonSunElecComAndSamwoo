import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:share_plus/share_plus.dart';

/// Scrollable terminal-style log of BLE CLI traffic.
class LogConsole extends StatefulWidget {
  const LogConsole({
    super.key,
    required this.lines,
    this.shareTitle = 'IFTECH UPS Log',
    this.onClear,
    this.onFullScreen,
  });

  final List<String> lines;
  final String shareTitle;
  final VoidCallback? onClear;
  final VoidCallback? onFullScreen;

  @override
  State<LogConsole> createState() => _LogConsoleState();
}

class _LogConsoleState extends State<LogConsole> {
  final _controller = ScrollController();

  @override
  void didUpdateWidget(covariant LogConsole oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.lines.length != oldWidget.lines.length) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_controller.hasClients) {
          _controller.animateTo(
            _controller.position.maxScrollExtent,
            duration: const Duration(milliseconds: 200),
            curve: Curves.easeOut,
          );
        }
      });
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  Future<void> _share(String text) async {
    final box = context.findRenderObject() as RenderBox?;
    final origin = box != null
        ? box.localToGlobal(Offset.zero) & box.size
        : null;
    await Share.share(
      text,
      subject: widget.shareTitle,
      sharePositionOrigin: origin,
    );
  }

  @override
  Widget build(BuildContext context) {
    final text = widget.lines.join('\n');
    final canShare = text.trim().isNotEmpty;

    return Container(
      width: double.infinity,
      decoration: BoxDecoration(
        color: const Color(0xFF0F1419),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: const Color(0xFF2A3441)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(12, 8, 0, 4),
            child: Row(
              children: [
                Text(
                  'LOG',
                  style: TextStyle(
                    color: Colors.tealAccent.shade100,
                    fontSize: 12,
                    fontWeight: FontWeight.w600,
                    letterSpacing: 1.2,
                  ),
                ),
                const Spacer(),
                if (widget.onClear != null)
                  IconButton(
                    tooltip: '로그 지우기',
                    iconSize: 18,
                    color: Colors.white54,
                    onPressed: widget.onClear,
                    icon: const Icon(Icons.delete_outline),
                  ),
                IconButton(
                  tooltip: '공유 (카톡·문자 등)',
                  iconSize: 18,
                  color: Colors.white54,
                  onPressed: !canShare ? null : () async => _share(text),
                  icon: const Icon(Icons.share),
                ),
                IconButton(
                  tooltip: '복사',
                  iconSize: 18,
                  color: Colors.white54,
                  onPressed: !canShare
                      ? null
                      : () async {
                          await Clipboard.setData(ClipboardData(text: text));
                          if (context.mounted) {
                            ScaffoldMessenger.of(context).showSnackBar(
                              const SnackBar(
                                content: Text('로그를 복사했습니다'),
                                duration: Duration(seconds: 1),
                              ),
                            );
                          }
                        },
                  icon: const Icon(Icons.copy),
                ),
                if (widget.onFullScreen != null)
                  IconButton(
                    tooltip: '전체 화면',
                    iconSize: 18,
                    color: Colors.white54,
                    onPressed: widget.onFullScreen,
                    icon: const Icon(Icons.fullscreen),
                  ),
              ],
            ),
          ),
          Expanded(
            child: widget.lines.isEmpty
                ? const Center(
                    child: Text(
                      '장치 응답이 여기에 표시됩니다',
                      style: TextStyle(color: Colors.white38, fontSize: 13),
                    ),
                  )
                : ListView.builder(
                    controller: _controller,
                    padding: const EdgeInsets.fromLTRB(12, 0, 12, 12),
                    itemCount: widget.lines.length,
                    itemBuilder: (context, i) {
                      final line = widget.lines[i];
                      final isCmd = line.startsWith('>');
                      return Text(
                        line,
                        style: TextStyle(
                          fontFamily: 'monospace',
                          fontSize: 12.5,
                          height: 1.35,
                          color: isCmd
                              ? Colors.lightBlueAccent.shade100
                              : Colors.greenAccent.shade100,
                        ),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }
}

/// Full-screen log viewer — push as a route.
class FullScreenLogPage extends StatefulWidget {
  const FullScreenLogPage({
    super.key,
    required this.lines,
    this.title = 'LOG',
    this.shareTitle = 'IFTECH UPS Log',
    this.onClear,
  });

  final List<String> lines;
  final String title;
  final String shareTitle;
  final VoidCallback? onClear;

  @override
  State<FullScreenLogPage> createState() => _FullScreenLogPageState();
}

class _FullScreenLogPageState extends State<FullScreenLogPage> {
  final _controller = ScrollController();

  @override
  void didUpdateWidget(covariant FullScreenLogPage oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.lines.length != oldWidget.lines.length) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_controller.hasClients) {
          _controller.animateTo(
            _controller.position.maxScrollExtent,
            duration: const Duration(milliseconds: 200),
            curve: Curves.easeOut,
          );
        }
      });
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final lines = widget.lines;
    return Scaffold(
      backgroundColor: const Color(0xFF0F1419),
      appBar: AppBar(
        title: Text(widget.title),
        backgroundColor: const Color(0xFF0F1419),
        foregroundColor: Colors.tealAccent.shade100,
        actions: [
          if (widget.onClear != null)
            IconButton(
              tooltip: '로그 지우기',
              onPressed: widget.onClear,
              icon: const Icon(Icons.delete_outline),
            ),
          IconButton(
            tooltip: '공유',
            onPressed: lines.isEmpty
                ? null
                : () async {
                    final text = lines.join('\n');
                    final box = context.findRenderObject() as RenderBox?;
                    await Share.share(
                      text,
                      subject: widget.shareTitle,
                      sharePositionOrigin: box != null
                          ? box.localToGlobal(Offset.zero) & box.size
                          : null,
                    );
                  },
            icon: const Icon(Icons.share),
          ),
          IconButton(
            tooltip: '복사',
            onPressed: lines.isEmpty
                ? null
                : () async {
                    await Clipboard.setData(
                      ClipboardData(text: lines.join('\n')),
                    );
                    if (context.mounted) {
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(
                          content: Text('복사됨'),
                          duration: Duration(seconds: 1),
                        ),
                      );
                    }
                  },
            icon: const Icon(Icons.copy),
          ),
        ],
      ),
      body: lines.isEmpty
          ? const Center(
              child: Text(
                '로그가 비어 있습니다',
                style: TextStyle(color: Colors.white38),
              ),
            )
          : ListView.builder(
              controller: _controller,
              padding: const EdgeInsets.all(12),
              itemCount: lines.length,
              itemBuilder: (context, i) {
                final line = lines[i];
                final isCmd = line.startsWith('>');
                return Text(
                  line,
                  style: TextStyle(
                    fontFamily: 'monospace',
                    fontSize: 13,
                    height: 1.4,
                    color: isCmd
                        ? Colors.lightBlueAccent.shade100
                        : Colors.greenAccent.shade100,
                  ),
                );
              },
            ),
    );
  }
}
