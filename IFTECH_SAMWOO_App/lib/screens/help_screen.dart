import 'package:flutter/material.dart';

class HelpScreen extends StatelessWidget {
  const HelpScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
      length: 3,
      child: Scaffold(
        appBar: AppBar(
          title: const Text('도움말'),
          bottom: const TabBar(
            tabs: [
              Tab(text: '사용법'),
              Tab(text: 'FW 업데이트'),
              Tab(text: '빠른 명령'),
            ],
          ),
        ),
        body: const TabBarView(
          children: [
            _HelpScroll(children: [
              _H('IFTECH FW UPDATE '),
              _P(
                'IFTECH 장비와 Bluetooth LE로 연결해 Wi‑Fi·IP·이름·시간을 설정하고, '
                'Wi‑Fi OTA로 펌웨어를 올리는 앱입니다.',
              ),
              _H('화면 구성'),
              _Bullet('연결 / 해제 — 장비 BLE에 붙거나 끊습니다.'),
              _Bullet('로그(터미널 아이콘) — 오른쪽에서 스와이프해도 열립니다. CLI 칩이나 전송을 누르면 자동으로 열립니다.'),
              _Bullet('Wi‑Fi 설정 — OTA에 쓸 AP SSID와 비밀번호를 장비에 저장합니다.'),
              _Bullet('시간설정 Update — 휴대폰 시각을 장비 RTC에 넣습니다.'),
              _Bullet('장치 상세 — NAME, MAC. 연필은 이름, 랜 아이콘은 이더넷 IP.'),
              _Bullet('빠른 명령 — 자주 쓰는 CLI를 한 번에 보냅니다.'),
              _Bullet('직접 명령(CLI) — 한 줄을 직접 입력해 전송합니다.'),
              _H('이 앱이 돌아가는 휴대폰 핫스팟'),
              _P(
                '펌웨어 올릴 때 쓰는 AP가, 지금 이 앱을 실행 중인 휴대폰의 모바일 핫스팟이면 '
                '「주변 Wi‑Fi 검색」목록에 뜨지 않습니다. 같은 기기는 자기 핫스팟을 검색 결과로 보여 주지 않습니다.',
              ),
              _P(
                '그럴 때는 검색을 기다리지 말고 SSID 칸에 핫스팟 이름을 직접 입력하고, '
                '비밀번호를 넣은 뒤 「SSID / PASS 장비로 전송」하면 됩니다.',
              ),
              _H('장비 찾기'),
              _P(
                '연결을 누르면 검색합니다. 기본으로 이름에 UPS, BMS, IFTECH, IFT가 들어간 장비를 보여 줍니다. '
                '검색어를 넣으면 그 글자가 들어간 이름도 함께 찾습니다. 전체검색은 주변 BLE를 모두 표시합니다.',
              ),
            ]),
            _HelpScroll(children: [
              _H('준비'),
              _Bullet('휴대폰 모바일 핫스팟을 켜거나, 인터넷이 되는 현장 공유기를 씁니다. 보안은 WPA2를 권장합니다.'),
              _Bullet(
                '이 앱을 실행 중인 휴대폰의 핫스팟은 Wi‑Fi 검색에 나오지 않습니다. '
                'SSID와 비밀번호를 직접 입력한 뒤 장비로 전송하십시오.',
              ),
              _H('순서'),
              _P('1. 연결 — 장비를 고릅니다.'),
              _P('2. version — 지금 펌웨어 버전을 로그에서 확인합니다.'),
              _P('3. Wi‑Fi 설정 — AP를 고르거나 입력하고 비밀번호를 넣은 뒤 SSID / PASS 장비로 전송. 로그에 SSID : / PASS : 가 나오면 저장된 것입니다.'),
              _P('4. update — 확인하면 장비가 재부팅하고 BLE는 끊깁니다. 이후 Wi‑Fi로 서버에서 펌웨어를 받습니다.'),
              _P('5. 업데이트가 끝나면 다시 연결한 뒤 version. 값이 새 펌웨어와 같으면 완료입니다.'),
              _H('주의'),
              _P('update 중에는 전원을 뽑지 마십시오. 버전이 그대로면 핫스팟·SSID·암호·인터넷을 다시 확인하고 같은 순서를 반복합니다.'),
            ]),
            _HelpScroll(children: [
              _H('빠른 명령'),
              _P('칩을 누르면 해당 CLI를 장비로 보내고 로그가 열립니다. 주황·빨강은 장비를 재부팅하므로 확인 창이 나옵니다.'),
              _Cmd('help', '장비가 지원하는 CLI 목록을 응답합니다.'),
              _Cmd('version', '현재 펌웨어 버전을 표시합니다. 업데이트 전후에 비교합니다.'),
              _Cmd('status', '팩·동작 상태 요약을 요청합니다.'),
              _Cmd('ip', '저장된 이더넷 IP를 읽은 뒤 수정할 수 있습니다. 저장 후 reboot 해야 적용됩니다.'),
              _Cmd('mac', 'Wi‑Fi MAC과 이더넷 MAC을 읽습니다.'),
              _Cmd('name', '저장된 장치 이름을 읽은 뒤 바꿀 수 있습니다.'),
              _Cmd('ssid?', '장비에 저장된 OTA용 SSID/비밀번호를 읽어 화면에 채웁니다.'),
              _Cmd('time', '장비 RTC를 읽거나, 휴대폰 시각으로 설정합니다. 상단 Update와 같습니다.'),
              _Cmd('update', '저장된 Wi‑Fi로 접속해 펌웨어 OTA를 시작합니다. 재부팅되며 BLE가 끊깁니다.'),
              _Cmd('reboot', '장비만 다시 시작합니다. OTA는 하지 않습니다.'),
              _H('직접 명령 예'),
              _P('ssid 핫스팟이름'),
              _P('pass 암호   (개방 AP는 pass none)'),
              _P('ip 192.168.0.57 255.255.255.0 192.168.0.1'),
            ]),
          ],
        ),
      ),
    );
  }
}

class _HelpScroll extends StatelessWidget {
  const _HelpScroll({required this.children});
  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.fromLTRB(20, 16, 20, 32),
      children: children,
    );
  }
}

class _H extends StatelessWidget {
  const _H(this.text);
  final String text;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(top: 16, bottom: 8),
      child: Text(
        text,
        style: Theme.of(context).textTheme.titleMedium?.copyWith(
              color: const Color(0xFF2DE0C8),
              fontWeight: FontWeight.w700,
            ),
      ),
    );
  }
}

class _P extends StatelessWidget {
  const _P(this.text);
  final String text;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Text(text, style: Theme.of(context).textTheme.bodyMedium?.copyWith(height: 1.45)),
    );
  }
}

class _Bullet extends StatelessWidget {
  const _Bullet(this.text);
  final String text;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text('·  ', style: TextStyle(color: Color(0xFF2DE0C8), fontSize: 16)),
          Expanded(child: Text(text, style: Theme.of(context).textTheme.bodyMedium?.copyWith(height: 1.4))),
        ],
      ),
    );
  }
}

class _Cmd extends StatelessWidget {
  const _Cmd(this.name, this.desc);
  final String name;
  final String desc;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 88,
            child: Text(
              name,
              style: const TextStyle(
                fontWeight: FontWeight.w700,
                color: Color(0xFF2DE0C8),
              ),
            ),
          ),
          Expanded(child: Text(desc, style: Theme.of(context).textTheme.bodyMedium?.copyWith(height: 1.4))),
        ],
      ),
    );
  }
}
