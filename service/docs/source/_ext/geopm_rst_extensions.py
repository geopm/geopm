#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
from sphinx.util import logging
from sphinx import addnodes
import json
import re
import collections

logger = logging.getLogger(__name__)

def create_signal_description_paragraph(text, signal_regex):
    """Create a signal description in a docutils paragraph node. Automatically
    wrap any named signals/controls in hyperlinks to their descriptions.

    signal_regex is a regular expression that captures a signal name, used
    for automatic cross-reference links.
    """
    msr_definition_paragraph = nodes.paragraph()
    for text_part in re.split(signal_regex, text):
        match = re.match(signal_regex, text_part)
        if match:
            # This part of the description names a signal, so wrap the text in a
            # link to the expected docutils reference ID. The ID should exist
            # if the mentioned signal has a description in our documentation.

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
            # This is not a signal cross-reference, so just insert the text.
            msr_definition_paragraph += nodes.Text(text_part)
    return msr_definition_paragraph

class GeopmMsrJson(SphinxDirective):
    """Add a Sphinx directive to import a json definition of MSR descriptions.
    
    Examples:
    
    * Render all MSRs in a json file
        .. geopm-msr-json:: ../json_data/msr_data_arch.json
    * Render only signals
        .. geopm-msr-json:: ../json_data/msr_data_arch.json
          :no-controls:
    * Render only controls
        .. geopm-msr-json:: ../json_data/msr_data_arch.json
          :no-signals:
    """
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

        msr_regex = re.compile(r'(MSR::\w+:\w+)', flags=re.ASCII)
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
                    # Change from 'error' to 'info' if you ever need to make
                    # this a non-build-blocking check
                    logger.error('Missing a description for %s in %s',
                                 geopm_msr_name,
                                 json_path)
                msr_definition_body += create_signal_description_paragraph(description_text, msr_regex)

                # The MSR description is followed by a list of properties.
                msr_property_list = nodes.bullet_list()
                try:
                    aggregation_type = msr_field_data['aggregation']
                except KeyError:
                    aggregation_type = 'select_first'
                    logger.error('Missing an aggregation function for %s in %s',
                                 geopm_msr_name,
                                 json_path)
                description_items = [
                    ('Aggregation', aggregation_type),
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


class GeopmSysfsJson(SphinxDirective):
    """Add a Sphinx directive to import a json definition of SysfsIOGroup descriptions.
    
    Examples:
    
    * Render all attributes in a json file
        .. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
    * Render only signals
        .. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
          :no-controls:
    * Render only controls
        .. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
          :no-signals:
    * Render only signal aliases
        .. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
          :no-controls:
          :aliases:
    """
    has_content = False
    required_arguments = 2
    option_spec = {
        'no-signals': directives.flag,
        'no-controls': directives.flag,
        'aliases': directives.flag
    }

    def run(self):
        # Use relfn2path(...)[1] # to get its absolute path.
        driver_name = self.arguments[0]
        json_path = self.env.relfn2path(self.arguments[1])[1]

        no_controls = 'no-controls' in self.options
        no_signals = 'no-signals' in self.options
        do_aliases = 'aliases' in self.options

        if no_controls and no_signals:
            logger.error('Requested SysfsIOGroup information, but asked for neither signals nor controls.',
                         location=self.get_location())

        # Notify Sphinx that changes to the json file should cause a rebuild
        # of any docs that point this directive to that file.
        self.env.note_dependency(json_path)

        sysfs_documentation = dict()
        try:
            with open(json_path) as json_file:
                sysfs_documentation = json.load(json_file)
        except FileNotFoundError:
            logger.error('Unable to read %s', json_path, location=self.get_location())
            return []
        except json.decoder.JSONDecodeError as e:
            logger.error('Unable to parse %s. Error: %s', json_path, e, location=self.get_location())
            return []

        attribute_regex = re.compile('(' + driver_name + r'::\w+)', flags=re.ASCII)
        doc_nodes = nodes.definition_list()
        alias_map = collections.defaultdict(list)
        attribute_definitions = []
        for signal_base_name, attribute_properties in sysfs_documentation['attributes'].items():
            is_control = attribute_properties['writeable']
            is_signal = not is_control
            if (is_control and no_controls) or (is_signal and no_signals):
                continue
            geopm_attribute_name = f'{driver_name}::{signal_base_name}'
            attribute_definition_body = nodes.definition()

            # Create a global docutils ID so we can :ref:`<driver>-<attribute>` to it
            docutils_id = nodes.make_id(geopm_attribute_name)
            self.env.app.env.domaindata["std"]["labels"][docutils_id] = (
                self.env.docname, docutils_id, geopm_attribute_name)
            self.env.app.env.domaindata["std"]["anonlabels"][docutils_id] = (
                self.env.docname, docutils_id)

            # Signal/control names are formatted as ``literal`` terms in
            # our documentation
            attribute_definition_name = nodes.term()
            attribute_definition_name += nodes.literal(text=geopm_attribute_name, ids=[docutils_id], names=[docutils_id])

            # Signal/control descriptions start with a paragraph containing
            # the high-level description
            try:
                description_text = attribute_properties['description']
            except KeyError:
                description_text = f'TODO: Add a description to {self.arguments[0]}'
                # Change from 'error' to 'info' if you ever need to make
                # this a non-build-blocking check
                logger.error('Missing a description for %s in %s',
                             geopm_attribute_name,
                             json_path)
            attribute_definition_body += create_signal_description_paragraph(description_text, attribute_regex)

            # The signal description is followed by a list of properties.
            signal_property_list = nodes.bullet_list()
            description_items = [
                ('Maps to attribute', attribute_properties['attribute']),
                ('Aggregation', attribute_properties['aggregation']),
                ('Domain', 'Determined by GEOPM at time of query'),
                ('Format', attribute_properties['format']),
                ('Unit', attribute_properties['units'])
            ]
            if len(attribute_properties['alias']) > 0 and do_aliases:
                alias_map[attribute_properties['alias']].append(geopm_attribute_name)
            for item_name, item_value in description_items:
                bullet_node = nodes.list_item()
                item_body = nodes.paragraph()
                item_body += nodes.strong(text=item_name)
                item_body += nodes.Text(f': {item_value}')
                bullet_node += item_body
                signal_property_list += bullet_node
            attribute_definition_body += signal_property_list
            attribute_definition = nodes.definition_list_item()
            attribute_definition += attribute_definition_name
            attribute_definition += attribute_definition_body
            attribute_definitions.append(attribute_definition)

        if do_aliases:
            for alias, targets in alias_map.items():
                alias_definition_name = nodes.term()
                alias_definition_name += nodes.literal(text=alias)
                alias_definition_body = nodes.definition()
                alias_definition_body += create_signal_description_paragraph(f'Maps to: {", ".join(targets)}', attribute_regex)
                doc_nodes += alias_definition_name
                doc_nodes += alias_definition_body
        else:
            for attribute_definition in attribute_definitions:
                doc_nodes += attribute_definition
        return [doc_nodes]


def setup(app):
    app.add_directive('geopm-msr-json', GeopmMsrJson)
    app.add_directive('geopm-sysfs-json', GeopmSysfsJson)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
