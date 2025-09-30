#!/usr/bin/env python3

# ruff: noqa: PLW2901 `for` loop variable overwritten by assignment target
# ruff: noqa: N806 Variable in function should be lowercase
# ruff: noqa: T201 `print` found
# ruff: noqa: FLY002 Consider f-string instead of string join

from __future__ import annotations

import argparse
import os
import struct
import sys


class ObjConverter:
    """Converts a Wavefront OBJ file into a C header."""

    def __init__(
        self,
        *,
        output_file_base: str,
        switch_winding: bool,
        generate_inline: bool,
        mesh_resource_dir: str,
        use_posix_paths: str,
    ):
        self._switch_winding_enabled = switch_winding
        self.output_file_base = output_file_base
        self._generate_inline = generate_inline
        self._mesh_resource_dir = mesh_resource_dir
        self._use_posix_paths = use_posix_paths

    def process_obj_file(self, input_file: str):
        positions = []
        normals = []
        texcoords = []
        faces = []

        with open(input_file, "r") as f:
            for line in f:
                parts = line.strip().split()
                if not parts:
                    continue
                if parts[0] == "v":
                    positions.append([float(v) for v in parts[1:]])
                elif parts[0] == "vn":
                    normals.append([float(v) for v in parts[1:]])
                elif parts[0] == "vt":
                    texcoords.append([float(v) for v in parts[1:]])
                elif parts[0] == "f":
                    # A face is a list of vertex/texcoord/normal indices
                    face_indices = []
                    for part in parts[1:]:
                        # OBJ indices are 1-based, so subtract 1.
                        indices = [(int(i) - 1) if i else -1 for i in part.split("/")]
                        face_indices.append(indices)
                    if self._switch_winding_enabled:
                        face_indices.reverse()
                    faces.append(face_indices)

        # Triangulate faces
        triangulated_faces = []
        for face in faces:
            if len(face) == 3:
                triangulated_faces.append(face)
            elif len(face) == 4:
                # Triangulate quad by splitting it into two triangles
                t1 = [face[0], face[1], face[2]]
                t2 = [face[0], face[2], face[3]]
                triangulated_faces.append(t1)
                triangulated_faces.append(t2)
            else:
                # For now, only support triangles and quads
                print(
                    f"WARNING: Skipping face with {len(face)} vertices. Only triangles and quads are supported.",
                    file=sys.stderr,
                )

        # Flatten the data based on face indices
        flat_positions = []
        flat_normals = []
        flat_texcoords = []

        for face in triangulated_faces:
            for vertex_indices in face:
                flat_positions.extend(positions[vertex_indices[0]])
                if len(vertex_indices) > 1 and vertex_indices[1] != -1:
                    flat_texcoords.extend(texcoords[vertex_indices[1]])
                if len(vertex_indices) > 2 and vertex_indices[2] != -1:
                    flat_normals.extend(normals[vertex_indices[2]])

        flattened_values = {
            "POSITION": flat_positions,
            "NORMAL": flat_normals,
            "TEXCOORD": flat_texcoords,
        }

        mesh_name = os.path.splitext(os.path.basename(input_file))[0].replace(".", "_")
        num_vertices = len(flat_positions) // 3

        if self._generate_inline:
            self._write_mesh_header(mesh_name, flattened_values, None, num_vertices)
        else:
            self._write_mesh_resource(mesh_name, flattened_values, None, num_vertices)
        self._write_model_header(mesh_name)
        self._write_model_impl(mesh_name)

    def build_mesh_header_filename(self, mesh_name: str) -> str:
        artifact_root = os.path.basename(self.output_file_base)
        return f"{artifact_root}_{mesh_name.lower()}_mesh.h"

    def build_mesh_resource_filename(self, mesh_name: str) -> str:
        artifact_root = os.path.basename(self.output_file_base)
        return f"{artifact_root}_{mesh_name.lower()}.mesh"

    def build_model_header_filename(self, mesh_name: str) -> str:
        artifact_root = os.path.basename(self.output_file_base)
        return f"{artifact_root}_{mesh_name.lower()}_model.h"

    def build_model_class_name(self, mesh_name: str) -> str:
        prefix = "".join(x.title() for x in self.output_file_base.split("_"))
        return f"{prefix}{mesh_name.title()}Model"

    def build_header_guard(self, mesh_name: str, suffix: str) -> str:
        return f"_{self.output_file_base.upper()}_{mesh_name.upper()}_{suffix.upper()}_H_"

    def _write_mesh_resource(
        self,
        mesh_name: str,
        flattened_values: dict[str, list[float]],
        transform_matrix: list[float] | None,
        num_vertices: int,
    ):
        with open(self.build_mesh_resource_filename(mesh_name), "wb") as outfile:
            # 4-byte vertex count
            outfile.write(struct.pack("<i", num_vertices))

            position_values = flattened_values.get("POSITION", [])

            # 4-byte position count
            if len(position_values) // 3 != num_vertices:
                raise ValueError(f"Position count mismatch: expected {num_vertices}, got {len(position_values) // 3}")
            outfile.write(struct.pack("<i", len(position_values)))
            for offset in range(0, len(position_values), 3):
                vertex = [
                    position_values[offset],
                    position_values[offset + 2],
                    position_values[offset + 1],
                ]
                outfile.write(struct.pack("<fff", *vertex))

            normal_values = flattened_values.get("NORMAL", [])
            outfile.write(struct.pack("<i", len(normal_values)))
            for offset in range(0, len(normal_values), 3):
                vertex = [
                    normal_values[offset],
                    normal_values[offset + 2],
                    normal_values[offset + 1],
                ]
                outfile.write(struct.pack("<fff", *vertex))

            texcoord_values = flattened_values.get("TEXCOORD", [])
            outfile.write(struct.pack("<i", len(texcoord_values)))
            for value in texcoord_values:
                outfile.write(struct.pack("<f", value))

    def _write_mesh_header(
        self,
        mesh_name: str,
        flattened_values: dict[str, list[float]],
        transform_matrix: list[float] | None,
        num_vertices: int,
    ):
        TRANSFORMED_COMPONENTS = {"POSITION", "NORMAL"}
        YZ_SWAPPED_COMPONENTS = {"POSITION", "NORMAL"}
        ELEMENTS_PER_LINE = {
            "TEXCOORD": 2,
        }

        header_guard = self.build_header_guard(mesh_name, "mesh")
        with open(self.build_mesh_header_filename(mesh_name), "w") as outfile:
            outfile.write(
                "\n".join(
                    [
                        "// Created with obj_converter.py",
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

            print(f"static constexpr uint32_t kNumVertices = {num_vertices};", file=outfile)
            print("", file=outfile)
            print(f"#endif  // {header_guard}", file=outfile)

    def _write_model_header(self, mesh_name: str):
        with open(self.build_model_header_filename(mesh_name), "w") as outfile:
            header_guard = self.build_header_guard(mesh_name, "model")
            class_name = self.build_model_class_name(mesh_name)
            outfile.write(
                "\n".join(
                    [
                        "// Generated by obj_converter.py",
                        f"#ifndef {header_guard}",
                        f"#define {header_guard}",
                        "",
                        "#include <cstdint>",
                        "",
                        '#include "model_builder.h"',
                        '#include "xbox_math_types.h"',
                        "",
                        f"class {class_name} : public SolidColorModelBuilder {{",
                        " public:",
                        f"  {class_name}() : SolidColorModelBuilder() {{}}",
                        f"  {class_name}(const vector_t &diffuse, const vector_t &specular) : SolidColorModelBuilder(diffuse, specular) {{}}",
                        "",
                        "  [[nodiscard]] uint32_t GetVertexCount() const override;",
                        "",
                        " protected:",
                        "  [[nodiscard]] const float *GetVertexPositions() override;",
                        "  [[nodiscard]] const float *GetVertexNormals() override;",
                        "  [[nodiscard]] const float *GetVertexTexCoords();",
                    ]
                )
            )

            if not self._generate_inline:
                outfile.write(
                    "\n".join(
                        [
                            "  void ReleaseData() override {",
                            "    if (vertices_) {",
                            "      delete[] vertices_;",
                            "      vertices_ = nullptr;",
                            "    }",
                            "    if (normals_) {",
                            "      delete[] normals_;",
                            "      normals_ = nullptr;",
                            "    }",
                            "    if (texcoords_) {",
                            "      delete[] texcoords_;",
                            "      texcoords_ = nullptr;",
                            "    }",
                            "  }",
                            "",
                            "  float *vertices_{nullptr};",
                            "  float *normals_{nullptr};",
                            "  float *texcoords_{nullptr};",
                            "",
                        ]
                    )
                )

            outfile.write(
                "\n".join(
                    [
                        "};",
                        "",
                        f"#endif  // {header_guard}",
                    ]
                )
            )

    def _write_model_impl(self, mesh_name: str):
        model_header = self.build_model_header_filename(mesh_name)
        with open(f"{os.path.splitext(model_header)[0]}.cpp", "w") as outfile:
            class_name = self.build_model_class_name(mesh_name)
            outfile.write(
                "\n".join(
                    [
                        "// Generated by obj_converter.py",
                        "",
                        f'#include "{model_header}"',
                        "",
                    ]
                )
            )

            if self._generate_inline:
                mesh_header = self.build_mesh_header_filename(mesh_name)
                outfile.write(
                    "\n".join(
                        [
                            f'#include "{mesh_header}"',
                            "",
                            f"uint32_t {class_name}::GetVertexCount() const {{ return kNumVertices; }}",
                            f"const float* {class_name}::GetVertexPositions() {{ return kPosition; }}",
                            f"const float* {class_name}::GetVertexNormals() {{ return kNormal; }}",
                            f"const float* {class_name}::GetVertexTexCoords() {{ return kTexcoord; }}",
                            "",
                        ]
                    )
                )
            else:
                sep = "/" if self._use_posix_paths else "\\"
                if self._mesh_resource_dir:
                    parent_dir = self._mesh_resource_dir.replace("/", sep).replace("\\", sep)
                    mesh_resource = sep.join([parent_dir, self.build_mesh_resource_filename(mesh_name)])
                    mesh_resource = mesh_resource.replace("\\", "\\\\")
                else:
                    mesh_resource = self.build_mesh_resource_filename(mesh_name)

                outfile.write(
                    "\n".join(
                        [
                            "#include <stdio.h>",
                            "",
                            '#include "debug_output.h"',
                            "",
                            "#define READ(target, size) \\",
                            "    if (fread((target), (size), 1, fp) != 1) { \\",
                            f'      ASSERT(!"Failed to read from {mesh_resource}"); \\',
                            "    }",
                            "",
                            "#define READ_ARRAY(target, size, elements) \\",
                            "    if (fread((target), (size), (elements), fp) != (elements)) { \\",
                            f'      ASSERT(!"Failed to read from {mesh_resource}"); \\',
                            "    }",
                            "",
                            "#define SKIP(size) \\",
                            "    if (fseek(fp, (size), SEEK_CUR)) { \\",
                            f'      ASSERT(!"Failed to seek in {mesh_resource}"); \\',
                            "    }",
                            "",
                            f"uint32_t {class_name}::GetVertexCount() const {{",
                            f'  FILE *fp = fopen("{mesh_resource}", "rb");',
                            f'  ASSERT(fp && "Failed to open {mesh_resource}");',
                            "  uint32_t num_vertices;",
                            "  READ(&num_vertices, sizeof(num_vertices))",
                            "  fclose(fp);",
                            "  return num_vertices;",
                            "}",
                            "",
                            f"const float* {class_name}::GetVertexPositions() {{",
                            '  ASSERT(!vertices_ && "vertices_ already populated")',
                            f'  FILE *fp = fopen("{mesh_resource}", "rb");',
                            f'  ASSERT(fp && "Failed to open {mesh_resource}");',
                            "  SKIP(sizeof(uint32_t))",
                            "",
                            "  uint32_t num_positions;",
                            "  READ(&num_positions, sizeof(num_positions))",
                            "",
                            "  vertices_ = new float[num_positions];",
                            "  READ_ARRAY(vertices_, sizeof(*vertices_), num_positions)",
                            "  fclose(fp);",
                            "  return vertices_;",
                            "}",
                            "",
                            f"const float* {class_name}::GetVertexNormals() {{",
                            '  ASSERT(!normals_ && "normals_ already populated")',
                            f'  FILE *fp = fopen("{mesh_resource}", "rb");',
                            f'  ASSERT(fp && "Failed to open {mesh_resource}");',
                            "  SKIP(sizeof(uint32_t))",
                            "",
                            "  uint32_t num_positions;",
                            "  READ(&num_positions, sizeof(num_positions))",
                            "  SKIP(sizeof(float) * num_positions)",
                            "",
                            "  uint32_t num_normals;",
                            "  READ(&num_normals, sizeof(num_normals))",
                            "  normals_ = new float[num_normals];",
                            "  READ_ARRAY(normals_, sizeof(*normals_), num_normals)",
                            "  fclose(fp);",
                            "",
                            "  return normals_;",
                            "}",
                            "",
                            f"const float* {class_name}::GetVertexTexCoords() {{",
                            '  ASSERT(!texcoords_ && "texcoords_ already populated")',
                            f'  FILE *fp = fopen("{mesh_resource}", "rb");',
                            f'  ASSERT(fp && "Failed to open {mesh_resource}");',
                            "  SKIP(sizeof(uint32_t))",
                            "",
                            "  uint32_t num_positions;",
                            "  READ(&num_positions, sizeof(num_positions))",
                            "  SKIP(sizeof(float) * num_positions)",
                            "",
                            "  uint32_t num_normals;",
                            "  READ(&num_normals, sizeof(num_normals))",
                            "  SKIP(sizeof(float) * num_normals)",
                            "",
                            "  uint32_t num_texcoords;",
                            "  READ(&num_texcoords, sizeof(num_texcoords))",
                            "  texcoords_ = new float[num_texcoords];",
                            "  READ_ARRAY(texcoords_, sizeof(*texcoords_), num_texcoords)",
                            "  fclose(fp);",
                            "",
                            "  return texcoords_;",
                            "}",
                            "",
                        ]
                    )
                )


def _main(args):
    input_file = os.path.expanduser(args.obj_file)
    output_file = os.path.expanduser(args.output) if args.output else os.path.splitext(input_file)[0]

    processor = ObjConverter(
        output_file_base=output_file,
        switch_winding=not args.keep_winding,
        generate_inline=args.inline,
        mesh_resource_dir=args.mesh_resource_dir,
        use_posix_paths=args.use_posix_paths,
    )
    processor.process_obj_file(input_file)


if __name__ == "__main__":

    def _parse_args():
        parser = argparse.ArgumentParser()

        parser.add_argument(
            "obj_file",
            metavar="obj_path",
            help="Wavefront OBJ file to process.",
        )

        parser.add_argument(
            "--keep-winding",
            "-k",
            action="store_true",
            help="Keeps the winding order of triangles",
        )

        parser.add_argument(
            "--output",
            "-o",
            help="Provides the base name (no extension) of the files into which C output should be written. Defaults to the OBJ file without extension.",
        )

        parser.add_argument(
            "--inline",
            action="store_true",
            help="Generates source files for meshes instead of runtime-loaded resources.",
        )

        parser.add_argument(
            "--mesh-resource-dir",
            default="models",
            help="Sets the path at which resource files will be loaded at runtime.",
        )

        parser.add_argument("--use-posix-paths", action="store_true", help="Use POSIX-style paths in generated code.")

        return parser.parse_args()

    sys.exit(_main(_parse_args()))
