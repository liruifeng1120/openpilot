#!/usr/bin/env python3
from itertools import chain
import os
import re
from openpilot.common.basedir import BASEDIR
from openpilot.system.ui.lib.multilang import SYSTEM_UI_DIR, UI_DIR, TRANSLATIONS_DIR, multilang

LANGUAGES_FILE = os.path.join(str(TRANSLATIONS_DIR), "languages.json")
POT_FILE = os.path.join(str(TRANSLATIONS_DIR), "app.pot")
DP_POT_FILE = os.path.join(str(TRANSLATIONS_DIR), "dragonpilot.pot")


def fix_english_translation_file(po_file_path):
  """Fix English .po file to use msgid as msgstr for empty translations"""
  if not os.path.exists(po_file_path):
    return

  with open(po_file_path, 'r', encoding='utf-8') as f:
    lines = f.readlines()

  i = 0
  while i < len(lines):
    line = lines[i].strip()

    # Skip header and comments
    if line.startswith('#') or not line:
      i += 1
      continue

    # Look for msgid start
    if line.startswith('msgid'):
      # Parse msgid content
      msgid_content = ""

      if line == 'msgid ""':
        # Multi-line msgid
        i += 1
        while i < len(lines) and lines[i].strip().startswith('"'):
          # Extract content between quotes
          content_line = lines[i].strip()
          if len(content_line) > 2:  # More than just quotes
            msgid_content += content_line[1:-1]  # Remove outer quotes
          i += 1

        # Now we should be at msgstr
        if i < len(lines) and lines[i].strip().startswith('msgstr ""'):
          # Replace empty msgstr with msgid content
          lines[i] = f'msgstr "{msgid_content}"\n'
      else:
        # Single line msgid
        if len(line) > 7:  # More than just 'msgid '
          msgid_content = line[7:-1]  # Remove 'msgid "' and ending '"'

        # Look for msgstr on next line
        i += 1
        if i < len(lines) and lines[i].strip() == 'msgstr ""':
          # Replace empty msgstr with msgid content
          lines[i] = f'msgstr "{msgid_content}"\n'

      i += 1
    else:
      i += 1

  with open(po_file_path, 'w', encoding='utf-8') as f:
    f.writelines(lines)


def fix_english_translation():
  """Fix English translation files to use msgid as msgstr for empty translations"""
  # Fix app_en.po
  app_en_po_file = os.path.join(TRANSLATIONS_DIR, "app_en.po")
  fix_english_translation_file(app_en_po_file)

  # Fix dragonpilot_en.po
  dragonpilot_en_po_file = os.path.join(TRANSLATIONS_DIR, "dragonpilot_en.po")
  fix_english_translation_file(dragonpilot_en_po_file)


def update_translations():
  files = []
  for root, _, filenames in chain(os.walk(SYSTEM_UI_DIR),
                                  os.walk(os.path.join(BASEDIR, "dragonpilot")),
                                  os.walk(os.path.join(UI_DIR, "widgets")),
                                  os.walk(os.path.join(UI_DIR, "layouts")),
                                  os.walk(os.path.join(UI_DIR, "mici")),
                                  os.walk(os.path.join(UI_DIR, "onroad"))):
    for filename in filenames:
      if filename.endswith(".py"):
        files.append(os.path.relpath(os.path.join(root, filename), BASEDIR))

  # Create main translation file
  cmd = ("xgettext -L Python --keyword=tr --keyword=trn:1,2 --keyword=tr_noop --from-code=UTF-8 " +
         "--flag=tr:1:python-brace-format --flag=trn:1:python-brace-format --flag=trn:2:python-brace-format " +
         f"-D {BASEDIR} -o {POT_FILE} {' '.join(files)}")

  ret = os.system(cmd)
  assert ret == 0

  # Generate/update translation files for each language
  for name in multilang.languages.values():
    if os.path.exists(os.path.join(TRANSLATIONS_DIR, f"app_{name}.po")):
      cmd = f"msgmerge --update --no-fuzzy-matching --backup=none --sort-output {TRANSLATIONS_DIR}/app_{name}.po {POT_FILE}"
      ret = os.system(cmd)
      assert ret == 0
    else:
      cmd = f"msginit -l {name} --no-translator --input {POT_FILE} --output-file {TRANSLATIONS_DIR}/app_{name}.po"
      ret = os.system(cmd)
      assert ret == 0

  # Fix English translation to use msgid as msgstr
  fix_english_translation()

def update_dp_translations():
  files = []
  for root, _, filenames in os.walk(os.path.join(BASEDIR, "dragonpilot")):
    for filename in filenames:
      if filename.endswith(".py"):
        files.append(os.path.relpath(os.path.join(root, filename), BASEDIR))

  # Create main translation file
  cmd = ("xgettext -L Python --keyword=tr --keyword=trn:1,2 --keyword=tr_noop --from-code=UTF-8 " +
         "--flag=tr:1:python-brace-format --flag=trn:1:python-brace-format --flag=trn:2:python-brace-format " +
         f"-D {BASEDIR} -o {DP_POT_FILE} {' '.join(files)}")

  ret = os.system(cmd)
  assert ret == 0

  # Generate/update translation files for each language
  for name in multilang.languages.values():
    po_file = os.path.join(TRANSLATIONS_DIR, f"dragonpilot_{name}.po")
    mo_file = os.path.join(TRANSLATIONS_DIR, f"dragonpilot_{name}.mo")

    if os.path.exists(po_file):
      cmd = f"msgmerge --update --no-fuzzy-matching --backup=none --sort-output {po_file} {DP_POT_FILE}"
      ret = os.system(cmd)
      assert ret == 0
    else:
      cmd = f"msginit -l {name} --no-translator --input {DP_POT_FILE} --output-file {po_file}"
      ret = os.system(cmd)
      assert ret == 0

    # Compile .po to .mo
    cmd = f"msgfmt {po_file} -o {mo_file}"
    ret = os.system(cmd)
    assert ret == 0

if __name__ == "__main__":
  update_translations()
  update_dp_translations()
