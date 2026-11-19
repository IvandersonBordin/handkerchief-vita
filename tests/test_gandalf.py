import contextlib
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("gandalf", Path(__file__).resolve().parents[1] / "deploy/gandalf.py")
g = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g)


class Supervisor(unittest.TestCase):
    def setUp(self):
        stack = self.enterContext(contextlib.ExitStack())
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self.state = root / "state"
        self.state.mkdir()
        for attr, value in {"ROOT": root, "STATE": self.state, "BRANCH": {"bruno": "main"}}.items():
            stack.enter_context(patch.object(g, attr, value))
        self.tests = stack.enter_context(patch.object(g, "tests"))
        self.review = stack.enter_context(patch.object(g, "review", return_value={
            "decision": "approve", "reason": "testado", "release": True}))
        self.git = lambda *args: subprocess.check_output(["git", "-C", root, *args], text=True).strip()
        self.git("init", "-qb", "main")
        self.git("config", "user.name", "Teste")
        self.git("config", "user.email", "teste@localhost")
        self.git("commit", "-qm", "base", "--allow-empty")
        self.root = root
        self.head = self.commit("tools/code")
        (self.state / "bruno.schedule").write_text("espera 0")
        self.db = {"approved": {}, "tips": {}, "events": {}, "calls": []}

    def commit(self, name):
        path = self.root / name
        path.parent.mkdir(exist_ok=True)
        path.write_text("conteúdo")
        self.git("add", name)
        self.git("commit", "-qm", "entrega")
        self.git("update-ref", "refs/remotes/origin/main", "HEAD")
        return self.git("rev-parse", "HEAD")

    def tick(self, now=10000):
        g.tick(self.db, lambda: None, now)

    def test_release_once(self):
        (self.state / "bruno.pending").write_text(self.head + "\nverificar posição\n")
        self.tick()
        self.tick(20000)
        self.review.assert_called_once()
        self.assertTrue((self.state / "bruno.approved").exists())

    def test_lock_quota_and_cooldown(self):
        with g.lock("bruno"):
            self.tick()
        marker = self.state / "usage-limit"
        marker.touch()
        self.tick()
        marker.unlink()
        self.db["calls"] = [9900]
        self.tick()
        self.tests.assert_not_called()

    def test_failed_test_parked(self):
        self.tests.side_effect = ValueError("falha")
        self.tick()
        self.tick()
        self.tests.assert_called_once()
        self.review.assert_not_called()

    def test_journal(self):
        self.db["tips"]["bruno"] = self.head
        self.commit("docs/agent-progress.md")
        self.tick()
        self.review.assert_not_called()

    def test_dependency_and_hold(self):
        (self.state / "bruno.pending").write_text(self.head + "\nexaminar\ncarlos/abc\n")
        self.tick()
        self.review.assert_not_called()
        self.db["approved"]["carlos/abc"] = True
        self.review.return_value["decision"] = "hold"
        self.tick()
        self.review.assert_called_once()
        self.assertFalse((self.state / "bruno.approved").exists())

    def test_budget_and_interrupted_reservation(self):
        self.db["calls"] = list(range(8))
        self.tick(20000)
        self.tests.assert_not_called()
        self.review.side_effect = KeyboardInterrupt
        with self.assertRaises(KeyboardInterrupt):
            self.tick(100000)
        self.tick(105000)
        self.review.assert_called_once()

    def test_quota_from_reviewer(self):
        def quota(*args):
            (self.state / "usage-limit").touch()
            raise RuntimeError("cota")
        self.review.side_effect = quota
        self.tick()
        self.tick(20000)
        self.review.assert_called_once()
        (self.state / "usage-limit").unlink()
        self.review.side_effect = None
        self.tick(20001)
        self.assertEqual(self.review.call_count, 2)
