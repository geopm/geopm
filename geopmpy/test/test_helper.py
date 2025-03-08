#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
from unittest.mock import Mock, MagicMock
from importlib.abc import MetaPathFinder


def mock_new(dtype_str, target=None):
    """Mock implementation of ffi.new(). Returns strings or lists for array
    requests. Otherwise passes-through the given data.
    """
    if dtype_str == 'char[]' and isinstance(target, int):
        # Alloc a string with the given length
        return bytearray(b'x' * target)
    if dtype_str == 'char[]' and isinstance(target, bytes):
        # Alloc a string with the given length
        return bytearray(target) + b'\0'
    elif dtype_str.endswith('[]') and isinstance(target, int):
        # Alloc a list with the given length
        return [0] * target
    elif target is not None:
        # Pass through the given data
        return target
    else:
        return MagicMock(name=f'ffi.new({dtype_str}, target={target})')


def mock_string(string):
    return string.split(b'\0', 1)[0]  # Copy up to a nul terminator


mock_libgeopm = MagicMock(name='mock_libgeopm')
mock_libgeopm.ffi.new.side_effect = mock_new
mock_libgeopm.ffi.string.side_effect = mock_string

mock_libgeopmd = MagicMock(name='mock_libgeopmd')
mock_libgeopmd.ffi.new.side_effect = mock_new
mock_libgeopmd.ffi.string.side_effect = mock_string


# Create a "meta hook" (https://docs.python.org/3/reference/import.html)
# that intercepts attempts to import our binary dependencies. Replace those
# imports with mock interfaces
class MockImportFinder(MetaPathFinder):
    def find_spec(self, fullname, target, path=None):
        if fullname == '_libgeopm_py_cffi':
            # Access from tests with "from test_helper import mock_libgeopm"
            mock_spec = Mock()
            mock_spec.loader.create_module.return_value = mock_libgeopm
            return mock_spec
        elif fullname == '_libgeopmd_py_cffi':
            # Access from tests with "from test_helper import mock_libgeopmd"
            mock_spec = Mock()
            mock_spec.loader.create_module.return_value = mock_libgeopmd
            return mock_spec


_mock_import_finder = MockImportFinder()
_is_injected = False
def remove_mock_libs():
    global _is_injected
    if _is_injected:
        sys.meta_path.remove(_mock_import_finder)
        _is_injected = False

def inject_mock_libs():
    global _is_injected
    if not _is_injected:
        sys.meta_path.insert(0, _mock_import_finder)
        _is_injected = True

inject_mock_libs()
