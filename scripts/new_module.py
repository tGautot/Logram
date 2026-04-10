#!/usr/bin/python3

import os
import sys
import re

project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

module_src_path = os.path.join(project_root, "src/frontends/term/src/modules")
module_hdr_path = os.path.join(project_root, "src/frontends/term/include")
module_cmk_path = os.path.join(project_root, "src/frontends/term/CMakeLists.txt")
main_cpp_path   = os.path.join(project_root, "src/frontends/term/main.cpp")

hdr_template = """
class MODULE_NAME : public LogParserTerminalModule {
  public:
  void registerUserInputMapping(LogParserTerminal&) override;
  void registerUserActionCallback(LogParserTerminal&) override;
  void registerCommandCallback(LogParserTerminal&) override;
};
"""

src_template = """
#include "terminal_modules.hpp"

#define MY_ACTION 999

void MODULE_NAME::registerUserInputMapping(LogParserTerminal&){
  // My Action will be fired when the user enters AAA
  lpt.registerUserInputMapping("AAA", MY_ACTION);
}
void MODULE_NAME::registerUserActionCallback(LogParserTerminal&) {
  lpt.registerActionCallback([](user_action_t act, term_state_t& term_state, CachedFilteredFileNavigator* cfn)-> int{
    if(act == MY_ACTION){
      // Do stuff
      return 1;
    }
    return 0;
  });
}
void MODULE_NAME::registerCommandCallback(LogParserTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int{
    // TODO, check for the command you want to handle here (e.g cmd.find(":my_cmd") == 0)
    return 0;
  });
}
"""


if len(sys.argv) == 1:
  print("Usage: ./new_module.py <ModuleName>")
  print("  e.g. ./new_module.py TextSearch")
  print("  This will create TextSearchModule in all the right places.")
  exit()


name = sys.argv[1]

# Ensure the name ends with "Module"
if not name.endswith("Module"):
  class_name = name + "Module"
else:
  class_name = name

# Convert PascalCase to snake_case for the filename
# e.g. TextSearchModule -> text_search_module
snake_name = re.sub(r'(?<!^)(?=[A-Z])', '_', class_name).lower()
src_filename = snake_name + ".cpp"

# Variable name for main.cpp: lowercase initials of each word
# e.g. TextSearchModule -> tsm
var_name = ''.join(c for c in class_name if c.isupper()).lower()

# Check if source file already exists before touching anything
src_full_path = os.path.join(module_src_path, src_filename)
if os.path.exists(src_full_path):
  print(f"ERROR: {src_full_path} already exists, not doing any work...")
  exit(1)

print(f"Creating module: {class_name}")
print(f"  Source file: {src_filename}")
print(f"  Variable name: {var_name}")

hdr_decl = hdr_template.replace("MODULE_NAME", class_name)
src_content = src_template.replace("MODULE_NAME", class_name)

# 1. Add class declaration to terminal_modules.hpp
hdr_full_path = os.path.join(module_hdr_path, "terminal_modules.hpp")
with open(hdr_full_path, 'r') as f:
  hdr_content = f.read()

hdr_content = hdr_content.replace("\n#endif", hdr_decl + "\n#endif")

with open(hdr_full_path, 'w') as f:
  f.write(hdr_content)
print(f"  Updated {hdr_full_path}")

# 2. Create the .cpp source file
with open(src_full_path, 'w') as f:
  f.write(src_content)
print(f"  Created {src_full_path}")

# 3. Add source file to CMakeLists.txt
cmk_full_path = module_cmk_path
with open(cmk_full_path, 'r') as f:
  cmk_content = f.read()

# Insert after the last src/modules/ entry in PRIVATE sources
lines = cmk_content.split('\n')
insert_idx = None
for i, line in enumerate(lines):
  if line.strip().startswith("src/modules/"):
    insert_idx = i

if insert_idx is not None:
  indent = lines[insert_idx][:len(lines[insert_idx]) - len(lines[insert_idx].lstrip())]
  lines.insert(insert_idx + 1, f"{indent}src/modules/{src_filename}")
  cmk_content = '\n'.join(lines)
  with open(cmk_full_path, 'w') as f:
    f.write(cmk_content)
  print(f"  Updated {cmk_full_path}")
else:
  print(f"  WARNING: Could not find insertion point in CMakeLists.txt, add manually:")
  print(f"    src/modules/{src_filename}")

# 4. Add module instantiation to main.cpp
with open(main_cpp_path, 'r') as f:
  main_content = f.read()

module_block = f"""
  {class_name} {var_name};
  {var_name}.registerUserInputMapping(lpt);
  {var_name}.registerUserActionCallback(lpt);
  {var_name}.registerCommandCallback(lpt);
"""

main_content = main_content.replace("  lpt.loop();", module_block + "  lpt.loop();")

with open(main_cpp_path, 'w') as f:
  f.write(main_content)
print(f"  Updated {main_cpp_path}")

print(f"\nDone! Now go implement your module in {src_filename}")
