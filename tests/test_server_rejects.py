#!/usr/bin/env python3
"""Characterisation tests for server.py B/L reject helpers.

server.py remains Python-2 syntax (print statements, Queue, SocketServer),
so these tests load only the pure module-level helpers via a tiny
import-safe source extraction — no TCP server, no OpenGL, no
gameplay/wire changes.
"""

from __future__ import print_function

import os
import re
import unittest


SERVER_PY = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                         'server.py')

_HELPER_FUNCS = (
    'block_reject_message',
    'light_reject_message',
)


def _extract_assignment(source, name):
    """Return source text for a top-level NAME = ... assignment (multi-line ok)."""
    pattern = re.compile(
        r'^%s\s*=\s*' % re.escape(name), re.MULTILINE)
    match = pattern.search(source)
    if not match:
        raise RuntimeError('assignment not found in server.py: %s' % name)
    start = match.start()
    # End at the next top-level non-indented, non-empty line after the '=' line.
    lines = source[start:].splitlines(True)
    chunk = [lines[0]]
    for line in lines[1:]:
        if line.strip() == '':
            chunk.append(line)
            continue
        if line[0] not in (' ', '\t'):
            break
        chunk.append(line)
    return ''.join(chunk)


def _extract_function(source, name):
    """Return source text for a top-level def name(...): block."""
    pattern = re.compile(
        r'^def %s\s*\(' % re.escape(name), re.MULTILINE)
    match = pattern.search(source)
    if not match:
        raise RuntimeError('function not found in server.py: %s' % name)
    start = match.start()
    lines = source[start:].splitlines(True)
    chunk = [lines[0]]
    for line in lines[1:]:
        if line.startswith('def ') or line.startswith('class '):
            break
        # Stop before a later top-level assignment/import after blank gap? Keep
        # indented body and blank lines inside the function; stop on next
        # top-level statement that is not a decorator-less continuation.
        if (line.strip()
                and line[0] not in (' ', '\t', '#')
                and not line.startswith('@')):
            break
        chunk.append(line)
    return ''.join(chunk)


def load_reject_helpers():
    """Import-safe extraction of pure reject helpers from server.py.

    Avoids importing the Py2-only TCP server module under Python 3.
    """
    with open(SERVER_PY, 'r') as fp:
        source = fp.read()
    chunks = [
        _extract_assignment(source, 'AUTH_REQUIRED'),
        _extract_assignment(source, 'INDESTRUCTIBLE_ITEMS'),
        _extract_assignment(source, 'ALLOWED_ITEMS'),
    ]
    for name in _HELPER_FUNCS:
        chunks.append(_extract_function(source, name))
    namespace = {}
    exec(compile('\n'.join(chunks), SERVER_PY, 'exec'), namespace)
    for name in ('AUTH_REQUIRED', 'INDESTRUCTIBLE_ITEMS', 'ALLOWED_ITEMS') + _HELPER_FUNCS:
        if name not in namespace:
            raise RuntimeError('failed to exec helper symbol: %s' % name)
    return namespace


class ServerRejectHelpersTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ns = load_reject_helpers()
        # Keep function refs on the namespace only — assigning plain functions
        # as class attributes would bind them as methods (extra self arg).
        cls.allowed = cls.ns['ALLOWED_ITEMS']
        cls.indestructible = cls.ns['INDESTRUCTIBLE_ITEMS']

    def setUp(self):
        # Default in server.py; restore every test so AUTH toggles stay local.
        self.ns['AUTH_REQUIRED'] = True
        self.block_reject = self.ns['block_reject_message']
        self.light_reject = self.ns['light_reject_message']

    # --- block_reject_message: auth ---

    def test_block_rejects_unauthenticated_when_auth_required(self):
        self.assertEqual(
            self.block_reject(None, 10, 1, 0),
            'Only logged in users are allowed to build.')

    def test_block_allows_authenticated_place_on_empty(self):
        self.assertIsNone(self.block_reject(42, 10, 1, 0))

    def test_block_allows_unauthenticated_when_auth_not_required(self):
        self.ns['AUTH_REQUIRED'] = False
        self.assertIsNone(self.block_reject(None, 10, 1, 0))

    # --- block_reject_message: y bounds ---

    def test_block_rejects_y_zero(self):
        self.assertEqual(
            self.block_reject(1, 0, 1, 0),
            'Invalid block coordinates.')

    def test_block_rejects_y_negative(self):
        self.assertEqual(
            self.block_reject(1, -1, 1, 0),
            'Invalid block coordinates.')

    def test_block_rejects_y_above_255(self):
        self.assertEqual(
            self.block_reject(1, 256, 1, 0),
            'Invalid block coordinates.')

    def test_block_allows_y_1_and_255(self):
        self.assertIsNone(self.block_reject(1, 1, 1, 0))
        self.assertIsNone(self.block_reject(1, 255, 1, 0))

    # --- block_reject_message: allowed items ---

    def test_block_rejects_disallowed_item(self):
        # 16 is indestructible and intentionally absent from ALLOWED_ITEMS.
        self.assertNotIn(16, self.allowed)
        self.assertEqual(
            self.block_reject(1, 10, 16, 0),
            'That item is not allowed.')

    def test_block_rejects_gap_and_out_of_range_items(self):
        self.assertNotIn(24, self.allowed)
        self.assertEqual(
            self.block_reject(1, 10, 24, 0),
            'That item is not allowed.')
        self.assertNotIn(64, self.allowed)
        self.assertEqual(
            self.block_reject(1, 10, 64, 0),
            'That item is not allowed.')

    def test_block_allows_air_and_typical_place_items(self):
        self.assertIn(0, self.allowed)
        self.assertIn(1, self.allowed)
        self.assertIn(63, self.allowed)
        self.assertIsNone(self.block_reject(1, 10, 0, 3))  # break non-empty
        self.assertIsNone(self.block_reject(1, 10, 1, 0))  # place empty
        self.assertIsNone(self.block_reject(1, 10, 63, 0))

    # --- block_reject_message: empty / occupied ---

    def test_block_rejects_place_into_occupied(self):
        self.assertEqual(
            self.block_reject(1, 10, 1, 2),
            'Cannot create blocks in a non-empty space.')

    def test_block_rejects_break_already_empty(self):
        self.assertEqual(
            self.block_reject(1, 10, 0, 0),
            'That space is already empty.')

    def test_block_allows_place_into_empty_and_break_occupied(self):
        self.assertIsNone(self.block_reject(1, 10, 2, 0))
        self.assertIsNone(self.block_reject(1, 10, 0, 2))

    # --- block_reject_message: indestructible ---

    def test_block_rejects_destroy_indestructible(self):
        self.assertEqual(self.indestructible, set([16]))
        self.assertEqual(
            self.block_reject(1, 10, 0, 16),
            'Cannot destroy that type of block.')

    # --- light_reject_message ---

    def test_light_rejects_unauthenticated_when_auth_required(self):
        self.assertEqual(
            self.light_reject(None, 1, 8),
            'Only logged in users are allowed to build.')

    def test_light_allows_unauthenticated_when_auth_not_required(self):
        self.ns['AUTH_REQUIRED'] = False
        self.assertIsNone(self.light_reject(None, 1, 8))

    def test_light_rejects_when_no_block(self):
        self.assertEqual(
            self.light_reject(1, 0, 8),
            'Lights must be placed on a block.')

    def test_light_rejects_invalid_value_bounds(self):
        self.assertEqual(
            self.light_reject(1, 1, -1),
            'Invalid light value.')
        self.assertEqual(
            self.light_reject(1, 1, 16),
            'Invalid light value.')

    def test_light_allows_valid_values_on_block(self):
        self.assertIsNone(self.light_reject(1, 1, 0))
        self.assertIsNone(self.light_reject(1, 5, 15))
        self.assertIsNone(self.light_reject(99, 16, 8))


if __name__ == '__main__':
    unittest.main()
