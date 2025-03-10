#!/usr/bin/env python3

from __future__ import annotations

import argparse
import glob
import os
import shutil
import sys


def _build_relative_paths(resource_roots: list[str], resource_dirs: list[str]) -> dict[str, str]:
    ret: dict[str, str] = {}

    for directory in resource_dirs:
        longest_match = -1
        rootless = ""

        for root in resource_roots:
            root_len = len(root)

            if directory.startswith(root) and root_len > longest_match:
                rootless = directory[root_len + 1 :]
                longest_match = root_len

        ret[directory] = rootless
    return ret


def _copy_if_needed(local_file_path: str, target_file_path: str) -> bool:
    file_stats = os.stat(local_file_path)
    file_size = file_stats.st_size
    modification_time = file_stats.st_mtime

    target_file_dir = os.path.dirname(target_file_path)
    if target_file_dir and not os.path.isdir(target_file_dir):
        os.makedirs(target_file_dir)

    if os.path.isfile(target_file_path):
        existing_stats = os.stat(target_file_path)
        if file_size == existing_stats.st_size and modification_time == existing_stats.st_mtime:
            return False

    shutil.copy2(local_file_path, target_file_path)
    return True


def _sync_resource_dir(local_resource_path: str, target_path: str, target_ignore_files: set[str], *, recursive: bool = True) -> set[str]:
    source_files = glob.glob(f"{local_resource_path}/**", recursive=recursive)
    existing_files = set(glob.glob(f"{target_path}/**", recursive=recursive)) if os.path.isdir(target_path) else set()

    os.makedirs(target_path, exist_ok=True)

    matched_files: set[str] = set()

    ret: set[str] = set()
    for file in source_files:
        if not os.path.isfile(file):
            continue

        relative_path = os.path.relpath(file, local_resource_path)
        target_file_path = os.path.join(target_path, relative_path)
        if target_file_path in target_ignore_files:
            continue
        matched_files.add(target_file_path)
        if _copy_if_needed(file, target_file_path):
            stat_results = os.stat(target_file_path)
            ret.add(f"{target_file_path}={stat_results.st_mtime}")

    extra_files = existing_files - matched_files
    extra_files -= target_ignore_files
    for extra_file in extra_files:
        if os.path.isfile(extra_file):
            os.unlink(extra_file)
            ret.add(extra_file)

    return ret


def perform_sync(output_dir: str, receipt_path: str | None, resource_roots: list[str], resource_dirs: list[str], ignore_files: list[str]) -> int:
    resource_path_to_relative_path = _build_relative_paths(resource_roots, resource_dirs)

    target_ignore_files = {os.path.join(output_dir, file) for file in ignore_files}

    files_modified: set[str] = set()
    for source_path, relative_target in resource_path_to_relative_path.items():
        new_files_modified = _sync_resource_dir(
            local_resource_path=source_path, target_path=os.path.join(output_dir, relative_target), target_ignore_files=target_ignore_files
        )
        files_modified.update(new_files_modified)

    if receipt_path:
        if files_modified or not os.path.exists(receipt_path):
            os.makedirs(os.path.dirname(receipt_path), exist_ok=True)
            with open(receipt_path, "w") as outfile:
                outfile.write(f"Modified\n{files_modified}")

    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "output_dir",
        help="Directory into which modified resources will be copied",
    )
    parser.add_argument(
        "--receipt",
        help="Path to a file that should be touched if any files were copied."
    )
    parser.add_argument(
        "--resource-roots",
        "-r",
        nargs="+",
        help="Prefixes which will be removed from any --resource-dir arguments when copying.",
    )
    parser.add_argument(
        "--resource-dirs",
        "-d",
        nargs="+",
        help="Directories that should be searched recursively for resource files that should be copied into the output-dir.",
    )
    parser.add_argument(
        "--ignore-files", "-i",
        nargs="+",
        help="Relative paths from `output_dir` to files that should be ignored."
    )

    args = parser.parse_args()
    return perform_sync(args.output_dir, args.receipt, args.resource_roots, args.resource_dirs, args.ignore_files)


if __name__ == "__main__":
    sys.exit(main())
