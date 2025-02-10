#!/usr/bin/env python3

# ruff: noqa: N817 CamelCase imported as acronym

from __future__ import annotations

import argparse
import json
import logging
import os
import re
import sys
import xml.etree.ElementTree as ET
from typing import Any, NamedTuple

logger = logging.getLogger(__name__)

SUITE_NAME_INITIALIZER_RE = re.compile(r'.*?"([^"]+)"')


class TestSuiteDescriptor(NamedTuple):
    """Describes one of the nxdk_pgraph_tests test suites."""

    suite_name: str
    class_name: str
    description: list[str]
    source_file: str
    source_file_line: int

    def to_obj(self) -> dict[str, Any]:
        return {
            "suite": self.suite_name,
            "class": self.class_name,
            "description": self.description,
            "source_file": self.source_file,
            "source_file_line": self.source_file_line,
        }


class TestSuiteDescriptorReader:
    """Loads test suite descriptors from the Doxygen XML."""

    def __init__(self, descriptor_base_path: str):
        self.descriptor_base_path = descriptor_base_path

    def _process_possible_test_descriptor(self, index_node: ET.Element) -> TestSuiteDescriptor | None:
        detailed_descriptor_path = index_node.get("refid")
        if not detailed_descriptor_path:
            return None

        class_name = ""
        name_element = index_node.find("name")
        if name_element is None:
            logger.debug("Failed to find class name for node %s", index_node)
            return None
        class_name = name_element.text
        if not class_name:
            logger.debug("Failed to find class name for node %s", index_node)
            return None

        detailed_descriptor_url = f"{detailed_descriptor_path}.xml"

        doxygen_root = self._load_xml(detailed_descriptor_url)
        if doxygen_root is None:
            return None
        class_descriptor = doxygen_root.find("compounddef")
        if class_descriptor is None:
            return None

        base_class = class_descriptor.find("basecompoundref")
        if base_class is None or base_class.text != "TestSuite":
            return None

        element = class_descriptor.find("compoundname")
        if element is None:
            logger.warning("Failed to find compoundname for %s", class_name)

        element = class_descriptor.find("location")
        if element is None:
            logger.warning("Failed to find location for %s", class_name)
            class_definition_file = ""
            class_definition_line = -1
        else:
            class_definition_file = element.get("file", "")
            class_definition_line = int(element.get("line", -1))

        detailed_description = []
        element = class_descriptor.find("detaileddescription")
        if element is None:
            logger.warning("No detailed description for test suite %s", class_name)
        else:
            detailed_description = [item.strip() for item in element.itertext() if item.strip()]

        suite_name = class_name
        # Look for a private static variable named kSuiteName which overrides the suite name used when saving artifacts.
        for section in class_descriptor.iter("sectiondef"):
            if section.get("kind") != "private-static-attrib":
                continue

            for member in section.iter("memberdef"):
                element = member.find("name")
                if element is None or element.text != "kSuiteName":
                    continue
                element = member.find("initializer")
                if element is None:
                    logger.warning("Found kSuiteName definition without an initializer")
                    continue

                match = SUITE_NAME_INITIALIZER_RE.match(element.text)
                if not match:
                    logger.warning("Found kSuiteName definition with unparseable initializer '%s'", element.text)
                    continue
                suite_name = match.group(1)

        return TestSuiteDescriptor(
            suite_name=suite_name,
            class_name=class_name,
            description=detailed_description,
            source_file=class_definition_file,
            source_file_line=class_definition_line,
        )

    def _process_index(self, root_node: ET.Element) -> list[TestSuiteDescriptor]:
        ret: list[TestSuiteDescriptor] = []

        for element in root_node:
            if element.get("kind") != "class":
                continue

            descriptor = self._process_possible_test_descriptor(element)
            if descriptor:
                ret.append(descriptor)

        return ret

    def _load_xml(self, path: str) -> ET.Element | None:
        try:
            tree = ET.parse(f"{self.descriptor_base_path}/{path}")  # noqa: S314 - data is generated from a known process and trusted.
            return tree.getroot()
        except FileNotFoundError:
            logger.exception("Failed to load XML file '%s'", path)
            return None

    def process(self) -> list[TestSuiteDescriptor]:
        """Loads the test suite descriptors from the nxdk_pgraph_tests project."""

        root = self._load_xml("index.xml")
        if root is None:
            return []

        return self._process_index(root)


def _write_descriptors(outfile_path: str, descriptors: list[TestSuiteDescriptor]) -> None:
    os.makedirs(os.path.dirname(outfile_path), exist_ok=True)

    result = {"test_suites": [item.to_obj() for item in descriptors]}

    with open(outfile_path, "w") as outfile:
        json.dump(result, outfile, indent=2, sort_keys=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--verbose",
        "-v",
        help="Enables verbose logging information",
        action="store_true",
    )
    parser.add_argument(
        "xml_dir",
        help="Directory including XML output from running Doxygen",
    )
    parser.add_argument(
        "--output-dir",
        help="Directory into which the nxdk_pgraph_tests_registry.json file will be generated (defaults to xml_dir)",
    )

    args = parser.parse_args()

    if not args.output_dir:
        args.output_dir = args.xml_dir
    output_file = os.path.join(args.output_dir, "nxdk_pgraph_tests_registry.json")

    descriptors = TestSuiteDescriptorReader(args.xml_dir).process()
    if not descriptors:
        logger.warning("No descriptors generated.")
        if os.path.isfile(output_file):
            os.unlink(output_file)
        return 0

    _write_descriptors(output_file, descriptors)

    return 0


if __name__ == "__main__":
    sys.exit(main())
