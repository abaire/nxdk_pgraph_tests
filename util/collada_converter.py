#!/usr/bin/env python3

# ruff: noqa: PLW2901 `for` loop variable overwritten by assignment target
# ruff: noqa: N806 Variable in function should be lowercase
# ruff: noqa: T201 `print` found
# ruff: noqa: S314 Using `xml` to parse untrusted data is known to be vulnerable to XML attacks; use `defusedxml` equivalents

from __future__ import annotations

import argparse
import os
import sys
from xml.etree import ElementTree

SCHEMA = "{http://www.collada.org/2005/11/COLLADASchema}"


def convert(data: str) -> list[float]:
    return [float(x) for x in data.split(" ")]


def matrix_mult_vertex(matrix, vertex):
    a = matrix
    b = [vertex[0], vertex[1], vertex[2], 1.0]
    ret = [0.0, 0.0, 0.0, 1.0]
    ret[0] = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]
    ret[1] = a[4] * b[0] + a[5] * b[1] + a[6] * b[2] + a[7] * b[3]
    ret[2] = a[8] * b[0] + a[9] * b[1] + a[10] * b[2] + a[11] * b[3]
    ret[3] = a[12] * b[0] + a[13] * b[1] + a[14] * b[2] + a[15] * b[3]
    return ret


def transform(values, transform_matrix):
    ret = []
    for offset in range(0, len(values), 3):
        vertex = values[offset : offset + 3]
        transformed = matrix_mult_vertex(transform_matrix, vertex)
        inv_w = 1.0 / transformed[3]
        transformed = [v * inv_w for v in transformed[0:3]]
        ret.extend(transformed)
    return ret


class ColadaConverter:
    """Converts a Collada DAE file into a C header."""

    def __init__(self, output_file_base: str, *, switch_winding: bool):
        self._switch_winding_enabled = switch_winding
        self._transforms: dict[str, list[float]] = {}
        self.output_file_base = output_file_base

    def switch_winding(self, indices):
        if not self._switch_winding_enabled:
            return indices

        ret = []
        for i in range(0, len(indices), 3):
            ret.append(indices[i])
            ret.append(indices[i + 2])
            ret.append(indices[i + 1])
        return ret

    def add_transform(self, transform):
        target = transform.find(f"{SCHEMA}instance_geometry")
        target_geometry = target.attrib["url"][1:]  # Drop the leading '#'

        matrix = transform.find(f"{SCHEMA}matrix")
        matrix_values = convert(matrix.text)

        self._transforms[target_geometry] = matrix_values

    def process_mesh_vertex_bindings(self, mesh) -> dict[str, dict[str, str]]:
        """Processes the 'vertices' node from the given mesh, returns {vertex_id: {semantic_field: source_id}}."""
        vertices = mesh.find(f"{SCHEMA}vertices")
        vertices_id = vertices.attrib["id"]

        ret = {}
        inputs = vertices.findall(f"{SCHEMA}input")
        for input_decl in inputs:
            ret[input_decl.attrib["semantic"]] = input_decl.attrib["source"]
        return {vertices_id: ret}

    def build_vertices(
            self,
            processed_sources: dict[str, list],
            vertex_bindings: dict[str, dict[str, str]],
            source_sizes: dict[str, int],
    ) -> dict[str, dict[str, list]]:
        """Joins vertex bindings and processed sources to return {vertex_id: {semantic_field: values}}."""

        ret = {}
        for vertex_id, field_bindings in vertex_bindings.items():
            resolved_bindings = {}

            for semantic_id, source_id in field_bindings.items():
                # Remove the leading '#'
                source_id = source_id[1:]

                values = processed_sources[source_id]
                resolved_bindings[semantic_id] = values

                source_sizes[semantic_id] = source_sizes.get(source_id)

            ret[vertex_id] = resolved_bindings

        return ret

    def split_indices(self, merged_indices: list[int], num_inputs: int) -> list[list[int]]:
        """Splits merged indices into a list of component indices."""
        ret = [[] for _ in range(num_inputs)]

        for record in range(0, len(merged_indices), num_inputs):
            for i in range(num_inputs):
                ret[i].append(merged_indices[record + i])
        return ret

    def process_mesh_triangles_input_bindings(self, mesh) -> dict[str, tuple[str, list[int]]]:
        """Processes the triangles inputs into a list of {semantic_type: (source, [index])}."""
        triangles = mesh.find(f"{SCHEMA}triangles")
        indices = triangles.find(f"{SCHEMA}p").text
        indices = [int(x) for x in indices.split(" ")]

        ret = {}
        inputs = triangles.findall(f"{SCHEMA}input")
        index_list = self.split_indices(indices, len(inputs))

        for input_decl in inputs:
            source = input_decl.attrib["source"]
            offset = int(input_decl.attrib["offset"])
            ret[input_decl.attrib["semantic"]] = source, index_list[offset]
        return ret

    def _flatten_indices(self, value_index_mappings):
        """Flattens the VERTEX structs within the given {component: (source, index)} mapping such that VERTEX members are siblings of anty other components."""
        flattened_indices = {}
        for key, value in value_index_mappings.items():
            if key != "VERTEX":
                flattened_indices[key] = value
                continue

            vertex_components, vertex_indices = value
            for component, vals in vertex_components.items():
                flattened_indices[component] = vals, vertex_indices
        return flattened_indices

    def _flatten_values(self, flattened_indices, components: dict[str, int]):
        """Convert from {component: (values, index)} to {component: values} by resolving indexing and filtering against the given `components`."""

        ret = {}
        for component, (values, index) in flattened_indices.items():
            stride = components.get(component)
            if not stride:
                continue

            resolved_values = []
            for idx in index:
                offset = idx * stride
                resolved_values.extend(values[offset : offset + stride])
            ret[component] = resolved_values

        return ret

    def flatten_values(
            self,
            components: set[str],
            sources: dict[str, float],
            source_sizes: dict[str, int],
            indices: dict[str, tuple[str, list[int]]],
    ) -> dict[str, list[float]]:
        """Returns {component: [values]}."""
        # Convert the indices from {component: (source, index)} to {component: (values, index)}.
        value_index_mappings = {}
        for component, (source, index_values) in indices.items():
            # Remove the leading '#'
            source = source[1:]
            source_sizes[component] = source_sizes.get(source)
            value_index_mappings[component] = sources[source], self.switch_winding(index_values)

        # Flatten the vertex components to produce a map of {component: (values, index)} without the VERTEX wrappers.
        flattened_indices = self._flatten_indices(value_index_mappings)

        component_sizes = {k: v for k, v in source_sizes.items() if k in components}
        return self._flatten_values(flattened_indices, component_sizes)

    def process_geometry(self, geometry):
        geometry_id = geometry.attrib["id"]
        meshes = geometry.findall(f"{SCHEMA}mesh")
        transform = self._transforms.get(geometry_id)
        for mesh in meshes:
            self.process_mesh(mesh, transform)

    def process_mesh(self, mesh, transform_matrix: list[float] | None):
        processed_sources = {}
        source_sizes = {}
        for source in mesh.findall(f"./{SCHEMA}source"):
            source_id = source.attrib["id"]
            values_str = source.find(f"{SCHEMA}float_array").text
            values = convert(values_str)
            processed_sources[source_id] = values

            accessor = source.find(f"{SCHEMA}technique_common/{SCHEMA}accessor")
            source_sizes[source_id] = int(accessor.attrib["stride"])

        # Extract vertex struct info and merge into the processed sources.
        vertex_bindings = self.process_mesh_vertex_bindings(mesh)
        vertices = self.build_vertices(processed_sources, vertex_bindings, source_sizes)
        for key, value in vertices.items():
            processed_sources[key] = value

        # Split out per-component indices.
        split_indices = self.process_mesh_triangles_input_bindings(mesh)

        # Convert to flattened, filtered array.
        DESIRED_COMPONENTS = {"POSITION", "NORMAL", "TEXCOORD"}
        flattened_values = self.flatten_values(DESIRED_COMPONENTS, processed_sources, source_sizes, split_indices)

        self._write_mesh_header(flattened_values, transform_matrix, mesh)
        self._write_model_header()
        self._write_model_impl()

    def _write_mesh_header(
            self, flattened_values: dict[str, list[float]], transform_matrix: list[float], mesh: ElementTree.Element
    ):
        TRANSFORMED_COMPONENTS = {"POSITION", "NORMAL"}
        YZ_SWAPPED_COMPONENTS = {"POSITION", "NORMAL"}
        ELEMENTS_PER_LINE = {
            "TEXCOORD": 2,
        }

        header_guard = f"_{self.output_file_base.upper()}_MESH_H_"
        with open(f"{self.output_file_base}_mesh.h", "w") as outfile:
            outfile.write(
                "\n".join(
                    [
                        "// Created with collada_converter.py",
                        "// clang-format off",
                        f"#ifndef {header_guard}",
                        f"#define {header_guard}",
                        "",
                        "",
                    ]
                )
            )

            for key, values in flattened_values.items():
                var_name = f"k{key.capitalize()}"
                print(f"static constexpr float {var_name}[] = {{", file=outfile)

                if key in TRANSFORMED_COMPONENTS and transform_matrix:
                    values = transform(values, transform_matrix)

                if key in YZ_SWAPPED_COMPONENTS:
                    for val_index in range(0, len(values), 3):
                        x = values[val_index]
                        y = values[val_index + 2]
                        z = values[val_index + 1]
                        print(f"  {x}f, {y}f, {z}f,", file=outfile)
                else:
                    elements_per_line = ELEMENTS_PER_LINE.get(key, 1)
                    for val_index in range(0, len(values), elements_per_line):
                        elements = [f"{val}f" for val in values[val_index : val_index + elements_per_line]]
                        print("  " + ", ".join(elements) + ",", file=outfile)
                print("};", file=outfile)
                print("", file=outfile)

            triangles = mesh.find(f"{SCHEMA}triangles")
            num_vertices = int(triangles.attrib["count"]) * 3
            print(f"static constexpr uint32_t kNumVertices = {num_vertices};", file=outfile)
            print("", file=outfile)
            print(f"#endif  // {header_guard}", file=outfile)

    @property
    def model_class_name(self) -> str:
        return f"{self.output_file_base.capitalize()}Model"

    def _write_model_header(self):
        with open(f"{self.output_file_base}_model.h", "w") as outfile:
            header_guard = f"_{self.output_file_base.upper()}_MODEL_H_"
            class_name = self.model_class_name
            outfile.write(
                "\n".join(
                    [
                        "// Generated by collada_converter.py",
                        f"#ifndef {header_guard}",
                        f"#define {header_guard}",
                        "",
                        '#include "model_builder.h"',
                        "",
                        f"class {class_name} : public SolidColorModelBuilder {{",
                        " public:",
                        f"  {class_name}() : SolidColorModelBuilder() {{}}",
                        f"  {class_name}(const vector_t &diffuse, const vector_t &specular) : SolidColorModelBuilder(diffuse, specular) {{}}",
                        "",
                        "  [[nodiscard]] uint32_t GetVertexCount() const override;",
                        "",
                        " protected:",
                        "   [[nodiscard]] const float *GetVertexPositions() const override;",
                        "   [[nodiscard]] const float *GetVertexNormals() const override;",
                        "};",
                        "",
                        f"#endif  // {header_guard}",
                    ]
                )
            )

    def _write_model_impl(self):
        artifact_root = os.path.basename(self.output_file_base)
        model_header = f"{artifact_root}_model.h"
        mesh_header = f"{artifact_root}_mesh.h"
        with open(f"{self.output_file_base}_model.cpp", "w") as outfile:
            class_name = self.model_class_name
            outfile.write(
                "\n".join(
                    [
                        "// Generated by collada_converter.py",
                        "",
                        f'#include "{model_header}"',
                        "",
                        f'#include "{mesh_header}"',
                        "",
                        f"uint32_t {class_name}::GetVertexCount() const {{ return kNumVertices; }}",
                        f"const float* {class_name}::GetVertexPositions() const {{ return kPosition; }}",
                        f"const float* {class_name}::GetVertexNormals() const {{ return kNormal; }}",
                        "",
                    ]
                )
            )


def _main(args):
    input_file = os.path.expanduser(args.colada_dae_file)
    output_file = os.path.expanduser(args.output) if args.output else os.path.splitext(input_file)[0]
    tree = ElementTree.parse(input_file)
    root = tree.getroot()

    geometries = root.findall(f".//{SCHEMA}library_geometries/{SCHEMA}geometry")

    processor = ColadaConverter(output_file_base=output_file, switch_winding=args.switch_winding)

    transforms = root.findall(f".//{SCHEMA}library_visual_scenes/{SCHEMA}visual_scene/{SCHEMA}node")
    for transform in transforms:
        processor.add_transform(transform)

    for geometry in geometries:
        processor.process_geometry(geometry)


if __name__ == "__main__":

    def _parse_args():
        parser = argparse.ArgumentParser()

        parser.add_argument(
            "colada_dae_file",
            metavar="dae_path",
            help="Collada file to process.",
        )

        parser.add_argument(
            "--switch-winding",
            "-w",
            action="store_true",
            help="Swaps the winding order of triangles",
        )

        parser.add_argument(
            "--output",
            "-o",
            help="Provides the base name (no extension) of the files into which C output should be written. Defaults to the DAE file without extension.",
        )

        return parser.parse_args()

    sys.exit(_main(_parse_args()))
