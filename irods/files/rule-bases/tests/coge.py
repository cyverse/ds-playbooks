#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# © 2025 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

"""Tests of coge.re rule logic."""

import unittest

import test_rules
from test_rules import IrodsTestCase

from irods.exception import CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME
from irods.models import Collection, CollectionAccess, CollectionUser


class TestCogeAcpostprocforcollcreate(IrodsTestCase):
    """Test the rule coge_acPostProcForCollCreate."""

    def test_coge_access(self):
        """Verify coge is given access to newly created coge_data collection."""
        coge_user = "coge"
        coge_coll_path = "/testing/home/rods/coge_data"
        try:
            self.irods.users.create(coge_user, "ds-service")
        except CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME:
            pass
        self.irods.collections.create(coge_coll_path)
        try:
            q = self.irods.query(CollectionAccess.name)
            q = q.filter(Collection.name == coge_coll_path, CollectionUser.name == coge_user)
            for res in q:
                self.assertEqual(res[CollectionAccess.name], "modify object")
                break
            else:
                self.fail(f"{coge_user} did not get access to {coge_coll_path}")
        finally:
            self.irods.collections.remove(coge_coll_path, force=True)
            self.irods.users.remove(coge_user)


class TestCogeAcpostprocforobjrename(IrodsTestCase):
    """Test the rule coge_acPostProcForObjRename."""

    @unittest.skip
    def test_coge_access(self):
        """Verify the rule gives coge access to the entity after it is renamed"""


@test_rules.unimplemented
class TestCogeDataObjCreated(IrodsTestCase):
    """Test the rule coge_dataObjCreated."""


if __name__ == "__main__":
    unittest.main()
