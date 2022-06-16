#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""Add a Sphinx directive to import a json definition of MSR descriptions.

Examples:

* Render all MSRs in a json file
    .. geopm-msr-json:: ../../src/msr_data_arch.json
* Render only signals
    .. geopm-msr-json:: ../../src/msr_data_arch.json
      :no-controls:
* Render only controls
    .. geopm-msr-json:: ../../src/msr_data_arch.json
      :no-signals:
"""
from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
from sphinx.util import logging
from sphinx import addnodes
import json
import re

logger = logging.getLogger(__name__)

MSR_REGEX = re.compile(r'(MSR::\w+:\w+)', flags=re.ASCII)

def create_msr_description_paragraph(text):
    """Create an MSR description in a docutils paragraph node. Automatically
    wrap any named MSR signals/controls in hyperlinks to their descriptions.
    """
    msr_definition_paragraph = nodes.paragraph()
    for text_part in re.split(MSR_REGEX, text):
        match = re.match(MSR_REGEX, text_part)
        if match:
            # This part of the description names an MSR, so wrap the text in a
            # link to the expected docutils reference ID. The ID should exist
            # if the mentioned MSR has a description in our documentation.

            # Use Sphinx's pending_xref instead of docutils' reference so that
            # Sphinx will automatically check for broken references.

            # This does not emit a very helpful error message (it just says
            # undefined label at the line where we are loading the json file).
            # We can implement our own context-aware xref checking and error
            # messaging if we create a set of sphinx roles/directives/domains,
            # e.g., to define and xref arbitrary signals/controls. Maybe
            # something nice to add later on.
            ref_node = addnodes.pending_xref(
                '', refdomain='std', refexplicit='False',
                reftarget=nodes.make_id(match.group(0)), reftype='ref',
                refwarn='False')
            ref_node += nodes.Text(match.group(0))
            msr_definition_paragraph += ref_node
        else:
            # This is not an MSR cross-reference, so just insert the text.
            msr_definition_paragraph += nodes.Text(text_part)
    return msr_definition_paragraph

class GeopmMsrJson(SphinxDirective):
    has_content = False
    required_arguments = 1
    option_spec = {
        'no-signals': directives.flag,
        'no-controls': directives.flag
    }

    def run(self):
        # Use relfn2path(...)[1] # to get its absolute path.
        json_path = self.env.relfn2path(self.arguments[0])[1]

        no_controls = 'no-controls' in self.options
        no_signals = 'no-signals' in self.options

        if no_controls and no_signals:
            logger.error('Requested MSR information, but asked for neither signals nor controls.',
                         location=self.get_location())

        # Notify Sphinx that changes to the json file should cause a rebuild
        # of any docs that point this directive to that file.
        self.env.note_dependency(json_path)

        msr_documentation = dict()
        try:
            with open(json_path) as json_file:
                msr_documentation = json.load(json_file)
        except FileNotFoundError:
            logger.error('Unable to read %s', json_path, location=self.get_location())
            return []
        except json.decoder.JSONDecodeError as e:
            logger.error('Unable to parse %s. Error: %s', json_path, e, location=self.get_location())
            return []

        msr_list = nodes.definition_list()
        for msr_base_name, msr_data in msr_documentation['msrs'].items():
            for msr_field_name, msr_field_data in msr_data['fields'].items():
                is_control = msr_field_data['writeable']
                is_signal = not is_control
                if (is_control and no_controls) or (is_signal and no_signals):
                    continue

                geopm_msr_name = f'MSR::{msr_base_name}:{msr_field_name}'
                msr_definition_body = nodes.definition()

                # Create a global docutils ID so we can :ref:`msr-some-field` to it
                docutils_id = nodes.make_id(geopm_msr_name)
                self.env.app.env.domaindata["std"]["labels"][docutils_id] = (
                    self.env.docname, docutils_id, geopm_msr_name)
                self.env.app.env.domaindata["std"]["anonlabels"][docutils_id] = (
                    self.env.docname, docutils_id)

                # Signal/control names are formatted as ``literal`` terms in
                # our documentation
                msr_definition_name = nodes.term()
                msr_definition_name += nodes.literal(text=geopm_msr_name, ids=[docutils_id], names=[docutils_id])

                # Signal/control descriptions start with a paragraph containing
                # the high-level description
                try:
                    description_text = msr_field_data['description']
                except KeyError:
                    description_text = f'TODO: Add a description to {self.arguments[0]}'
                    # TODO: Promote from 'info' to 'error' after we ratchet down the count
                    logger.info('Missing a description for %s in %s',
                                geopm_msr_name,
                                json_path, color='yellow')
                msr_definition_body += create_msr_description_paragraph(description_text)

                # The MSR description is followed by a list of properties.
                msr_property_list = nodes.bullet_list()
                description_items = [
                    ('Aggregation', msr_field_data.get('aggregation', 'select_first')),
                    ('Domain', msr_data['domain']),
                    ('Format', 'integer'),
                    ('Unit', msr_field_data['units'])
                ]
                for item_name, item_value in description_items:
                    bullet_node = nodes.list_item()
                    item_body = nodes.paragraph()
                    item_body += nodes.strong(text=item_name)
                    item_body += nodes.Text(f': {item_value}')
                    bullet_node += item_body
                    msr_property_list += bullet_node
                msr_definition_body += msr_property_list

                msr_definition = nodes.definition_list_item()
                msr_definition += msr_definition_name
                msr_definition += msr_definition_body

                msr_list += msr_definition
        return [msr_list]


def setup(app):
    app.add_directive('geopm-msr-json', GeopmMsrJson)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
