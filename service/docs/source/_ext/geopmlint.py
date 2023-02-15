#!/usr/bin/env python3
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
from typing import Any, Dict, Set

from docutils import nodes

from sphinx.application import Sphinx
from sphinx.builders import Builder
from sphinx.util import logging

import collections

logger = logging.getLogger(__name__)

EXPECTED_SIGNAL_PROPERTIES = ["Aggregation", "Domain", "Format", "Unit"]

# The following section order must be followed in geopm_pio_*.7.rst files,
# for sections that are present.
EXPECTED_PIO_SECTION_ORDER = ["Description", "Requirements", "Signals",
                              "Controls", "Aliases", "Signal Aliases",
                              "Control Aliases", "See Also"]

# The maximum number of characters to include when showing examples of
# offending text in an error message
MAX_CONTEXT_STRING_LENGTH = 20


class CheckGeopmStyleBuilder(Builder):
    name = "geopmlint"
    epilog = "The GEOPM linter does not generate any output files."

    def init(self) -> None:
        pass

    def get_outdated_docs(self) -> Set[str]:
        return self.env.found_docs

    def get_target_uri(self, docname: str, typ: str = None) -> str:
        return ""

    def prepare_writing(self, docnames: Set[str]) -> None:
        pass

    def _emit_error(self, location, message, *logger_args):
        """ Log an error and set the statuscode to 1.

        Args:
            location: Location of the error in the source code. E.g., may be a
                parser node, a docname, or a tuple of (docname, line).
            message (str): Format string to send to the error logger
            logger_args: Additional arguments (such as values to format) to
                send to the logger.
        """
        logger.error(message, *logger_args, location=location)
        self.app.statuscode = 1

    def write_doc(self, docname: str, doctree: nodes.Node) -> None:
        """ Check for GEOPM rst documentation conventions.
         - Signals should have definitions that list their properties for
           aggregation, domain, format, and unit.
         - Signal properties should be described in all-lowercase text.
         - geopm_pio_*.7 docs have an expected section hierarchy
        """
        sections_and_parents = list()

        for section_node in doctree.traverse(nodes.section):
            section_name = (section_node["names"][0] if
                            len(section_node["names"]) > 0 else None)
            parent_name = (section_node.parent["names"][0] if
                           len(section_node.parent["names"]) > 0 else None)

            if section_name is not None:
                sections_and_parents.append((section_name, parent_name))

            if section_name in ["signals", "controls"]:
                for definition_item_node in section_node.traverse(nodes.definition_list_item):
                    term_node = definition_item_node.next_node(nodes.term)
                    definition_node = definition_item_node.next_node(nodes.definition)

                    bullet_prefix_counts = collections.Counter()
                    for bullet_item in definition_node.traverse(nodes.list_item):
                        bullet_text = bullet_item.astext()
                        for prefix in EXPECTED_SIGNAL_PROPERTIES:
                            if bullet_text.startswith(f"{prefix}:"):
                                bullet_prefix_counts[prefix] += 1
                                bullet_value = bullet_text.split(":", 1)[-1].strip()
                                if not bullet_value.islower():
                                    if len(bullet_value) > MAX_CONTEXT_STRING_LENGTH:
                                        bullet_value = bullet_value[:MAX_CONTEXT_STRING_LENGTH] + "..."
                                    self._emit_error(
                                        bullet_item,
                                        "Value of the %s property in %s (%s) should be all lowercase",
                                        prefix, term_node.astext(), bullet_value)

                    if any(bullet_prefix_counts[field] != 1
                           for field in EXPECTED_SIGNAL_PROPERTIES):
                        self._emit_error(
                            definition_item_node,
                            "Definition of %s should have exactly 1 bullet point each for "
                            '"Aggregation:", "Domain:", "Format:", and "Unit:".',
                            term_node.astext())

        if docname.startswith('geopm_pio_') and docname.endswith('.7'):
            section_positions = {section.lower(): position for position, (section, _)
                                 in enumerate(sections_and_parents)}

            last_position = -1
            for section in EXPECTED_PIO_SECTION_ORDER:
                if section.lower() in section_positions:
                    position = section_positions[section.lower()]
                    if position > last_position:
                        last_position = position
                    else:
                        self._emit_error(
                            docname,
                            'The "%s" section is out of place. Expected order (may omit sections): %s',
                            section,
                            ", ".join(f'"{name}"' for name in EXPECTED_PIO_SECTION_ORDER))

            for section, parent in sections_and_parents:
                if section.lower() in ['signal aliases', 'control aliases'] and parent != 'aliases':
                    self._emit_error(
                        docname,
                        'The "%s" section must be a child of the "Aliases" section',
                        section)

    def finish(self) -> None:
        pass


def setup(app: Sphinx) -> Dict[str, Any]:
    app.add_builder(CheckGeopmStyleBuilder)

    return {
        "version": "builtin",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
