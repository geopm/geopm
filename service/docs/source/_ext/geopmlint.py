#!/usr/bin/env python3
from typing import Any, Dict, Set

from docutils import nodes

from sphinx.application import Sphinx
from sphinx.builders import Builder
from sphinx.util import logging

import collections

logger = logging.getLogger(__name__)

EXPECTED_SIGNAL_PROPERTIES = ["Aggregation", "Domain", "Format", "Unit"]


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

    def write_doc(self, docname: str, doctree: nodes.Node) -> None:
        """ Check for GEOPM rst documentation conventions.
         - Signals should have definitions that list their properties for
           aggregation, domain, format, and unit.
         - Signal properties should be described in all-lowercase text.
        """
        for section_node in doctree.traverse(nodes.section):
            if section_node["names"] in [["signals"], ["controls"]]:
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
                                    if len(bullet_value) > 15:
                                        bullet_value = bullet_value[:15] + "..."
                                    logger.error(
                                        "Value of the %s property in %s (%s) should be all lowercase",
                                        prefix, term_node.astext(), bullet_value, location=bullet_item)
                                    self.app.statuscode = 1

                    if any(bullet_prefix_counts[field] != 1
                           for field in EXPECTED_SIGNAL_PROPERTIES):
                        logger.error(
                            "Definition of %s should have exactly 1 bullet point each for "
                            '"Aggregation:", "Domain:", "Format:", and "Unit:".',
                            term_node.astext(), location=definition_item_node)
                        self.app.statuscode = 1

    def finish(self) -> None:
        pass


def setup(app: Sphinx) -> Dict[str, Any]:
    app.add_builder(CheckGeopmStyleBuilder)

    return {
        "version": "builtin",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
