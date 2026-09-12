Import("env")
import os
import re

"""
빌드 전 Version.h 의 VERSION patch(마지막 숫자)를 +1 한다.
예: "1.0.9" → "1.0.10"
"""

def bump_version_h():
    version_h_path = os.path.join(env.get("PROJECT_DIR"), "Version.h")
    if not os.path.exists(version_h_path):
        print("pre_build: Version.h 없음 — skip")
        return

    try:
        with open(version_h_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except Exception as e:
        print(f"pre_build: Version.h 읽기 실패: {e}")
        return

    changed = False
    new_lines = []
    for line in lines:
        # 활성 #define VERSION "x.y.z" 만 대상 (주석 처리된 줄은 제외)
        m = re.match(
            r'^(\s*#define\s+VERSION\s+")(\d+)\.(\d+)\.(\d+)(".*)$',
            line.rstrip("\r\n"),
        )
        if m and not changed:
            major, minor, patch = int(m.group(2)), int(m.group(3)), int(m.group(4))
            new_patch = patch + 1
            old_ver = f"{major}.{minor}.{patch}"
            new_ver = f"{major}.{minor}.{new_patch}"
            new_line = f"{m.group(1)}{new_ver}{m.group(5)}\n"
            new_lines.append(new_line)
            print(f"pre_build: VERSION {old_ver} -> {new_ver}")
            changed = True
        else:
            new_lines.append(line if line.endswith("\n") else line + "\n")

    if not changed:
        print("pre_build: 활성 #define VERSION 을 찾지 못함 — skip")
        return

    try:
        with open(version_h_path, "w", encoding="utf-8", newline="\n") as f:
            f.writelines(new_lines)
    except Exception as e:
        print(f"pre_build: Version.h 쓰기 실패: {e}")

print("=== pre_build.py ===")
bump_version_h()
