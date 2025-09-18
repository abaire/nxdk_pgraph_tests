#!/usr/bin/env python3

# ruff: noqa: T201 `print` found

import argparse
import os
import re
import sys

from jinja2 import Environment, FileSystemLoader


def to_camel_case(snake_str):
    return "".join(x.capitalize() for x in snake_str.lower().split("_"))


def get_paths(suite_name_snake):
    return {
        "h": os.path.join("src", "tests", f"{suite_name_snake}_tests.h"),
        "cpp": os.path.join("src", "tests", f"{suite_name_snake}_tests.cpp"),
    }


def create_files(suite_name_snake, suite_name_camel, suite_name_pretty):
    class_name = f"{suite_name_camel}Tests"
    paths = get_paths(suite_name_snake)
    header_path = paths["h"]
    cpp_path = paths["cpp"]
    header_name = os.path.basename(paths["h"])

    script_dir = os.path.dirname(os.path.realpath(__file__))
    loader = FileSystemLoader(os.path.join(script_dir, "templates"))
    env = Environment(loader=loader, autoescape=True)

    context = {
        "suite_name_snake": suite_name_snake,
        "suite_name_camel": suite_name_camel,
        "suite_name_pretty": suite_name_pretty,
        "class_name": class_name,
        "header_name": header_name,
    }

    # Create header
    template = env.get_template("test_suite.h.j2")
    content = template.render(context)
    with open(header_path, "w") as f:
        f.write(content)
    print(f"Created {header_path}")

    # Create cpp
    template = env.get_template("test_suite.cpp.j2")
    content = template.render(context)
    with open(cpp_path, "w") as f:
        f.write(content)
    print(f"Created {cpp_path}")


def update_cmakelists(suite_name_snake):
    cmake_path = os.path.join("src", "CMakeLists.txt")
    paths = get_paths(suite_name_snake)
    h_file = os.path.basename(paths["h"])
    cpp_file = os.path.basename(paths["cpp"])

    with open(cmake_path) as f:
        content = f.read()

    # Find the list of test files
    match = re.search(r"add_executable\(\s*nxdk_pgraph_tests\s*([^)]+)\)", content, re.DOTALL)
    if not match:
        print("Error: Could not find test list in CMakeLists.txt", file=sys.stderr)
        return False

    file_list_str = match.group(1)
    files = file_list_str.strip().split()
    files.append(f"tests/{cpp_file}")
    files.append(f"tests/{h_file}")
    files.sort()

    new_file_list_str = "\n        ".join(files)
    new_content = content.replace(file_list_str, f"{new_file_list_str}\n")

    with open(cmake_path, "w") as f:
        f.write(new_content)
    print(f"Updated {cmake_path}")
    return True


def update_main_cpp(suite_name_snake, suite_name_camel):
    main_cpp_path = os.path.join("src", "main.cpp")
    paths = get_paths(suite_name_snake)
    header_name = os.path.basename(paths["h"])
    class_name = f"{suite_name_camel}Tests"

    with open(main_cpp_path) as f:
        lines = f.readlines()

    # Find include block
    include_start_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include "tests/'):
            include_start_idx = i
            break

    if include_start_idx == -1:
        print("Error: Could not find include block in main.cpp", file=sys.stderr)
        return False

    include_end_idx = -1
    includes = []
    for i in range(include_start_idx, len(lines)):
        line = lines[i]
        if not line.strip().startswith('#include "tests/'):
            include_end_idx = i
            break
        includes.append(line)

    # Add new header include
    include_str = f'#include "tests/{header_name}"\n'
    includes.append(include_str)
    includes.sort()

    # Find REG_TEST block
    reg_begin_marker = "  // -- Begin REG_TEST --\n"
    reg_end_marker = "  // -- End REG_TEST --\n"
    reg_start_idx = -1
    reg_end_idx = -1

    for i in range(include_end_idx, len(lines)):
        line = lines[i]
        if reg_start_idx == -1 and reg_begin_marker in line:
            reg_start_idx = i + 1
        elif reg_end_idx == -1 and reg_end_marker in line:
            reg_end_idx = i
            break

    if reg_start_idx == -1 or reg_end_idx == -1:
        print("Error: Could not find REG_TEST markers in main.cpp", file=sys.stderr)
        return False

    reg_tests = lines[reg_start_idx:reg_end_idx]

    # Add new REG_TEST
    reg_test_str = f"  REG_TEST({class_name})\n"
    reg_tests.append(reg_test_str)
    reg_tests = sorted(reg_tests, key=str.lower)

    # Reconstruct the file
    new_lines = lines[:include_start_idx]
    new_lines.extend(includes)
    new_lines.extend(lines[include_end_idx:reg_start_idx])
    new_lines.extend(reg_tests)
    new_lines.extend(lines[reg_end_idx:])

    with open(main_cpp_path, "w") as f:
        f.writelines(new_lines)
    print(f"Updated {main_cpp_path}")
    return True


def main():
    parser = argparse.ArgumentParser(description="Create a new test suite.")
    parser.add_argument("suite_name", help="Snake case name of the test suite (e.g., 'my_new_tests').")
    args = parser.parse_args()

    suite_name_snake: str = args.suite_name
    if any(c.isupper() for c in suite_name_snake):
        print(f"Error: '{suite_name_snake}' appears to be camel cased. It must be of the form `foo_bar_tests`.")
        sys.exit(1)
    if suite_name_snake.endswith("_tests"):
        suite_name_snake = suite_name_snake[:-6]

    suite_name_camel = to_camel_case(suite_name_snake)
    suite_name_pretty = suite_name_camel

    create_files(suite_name_snake, suite_name_camel, suite_name_pretty)

    if not update_cmakelists(suite_name_snake):
        sys.exit(1)
    if not update_main_cpp(suite_name_snake, suite_name_camel):
        sys.exit(1)


if __name__ == "__main__":
    main()
