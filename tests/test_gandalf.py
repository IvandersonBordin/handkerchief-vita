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
            "decision": "approve", "reason": "testado", "next_action": "b03-edi"}))
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

    def test_idle_dispatch_without_model_and_single_use(self):
        self.tick()
        self.assertEqual(self.review.call_count, 1)
        (self.state / "gandalf.json").write_text(g.json.dumps(self.db))
        self.assertEqual(g.consume("bruno", self.root), 0)
        self.tick(20000)
        key = "bruno/" + self.head + "/b04-indices"
        self.assertIn(key, self.db["grants"])
        self.assertEqual(self.review.call_count, 1)
        (self.state / "gandalf.json").write_text(g.json.dumps(self.db))
        self.assertEqual(g.consume("bruno", self.root), 0)
        self.assertEqual(g.consume("bruno", self.root), 3)
        self.tick(30000)
        self.assertEqual(self.review.call_count, 1)
        self.assertNotIn("bruno/" + self.head + "/b04-indices-extra", self.db["grants"])

    def test_idle_needs_approved_review(self):
        self.review.return_value.update(decision="hold", next_action="hold")
        self.tick()
        self.tick(20000)
        self.assertEqual(self.review.call_count, 1)
        self.assertNotIn("grants", self.db)

    def test_catalog_avoids_repeated_grants(self):
        self.assertIn("b03-edi", g.available_actions(self.db, "bruno"))
        for i in range(3):
            g.grant(self.db, "bruno", str(i), "b03-edi", "")
        available = g.available_actions(self.db, "bruno")
        self.assertNotIn("b03-edi", available)
        self.assertIn("b04-indices", available)
        self.assertIn("b02-posicao", available)

    def test_repetitive_action_does_not_block_review(self):
        for i in range(3):
            g.grant(self.db, "bruno", str(i), "b03-edi", "")
        self.tick()
        self.assertEqual(self.db["approved"].get("bruno/" + self.head), True)
        self.assertNotIn("bruno/" + self.head, self.db["grants"])
        self.assertIn("b04-indices", self.review.call_args.args[4])

    def test_catalog_for_carlos_has_independent_task(self):
        self.assertIn("controles-a0", g.available_actions(self.db, "carlos"))

    def test_release_once(self):
        (self.state / "bruno.pending").write_text(self.head + "\nverificar posição\n")
        self.tick()
        self.tick(20000)
        self.review.assert_called_once()
        self.assertEqual(self.db["grants"]["bruno/" + self.head]["action"], "b03-edi")

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
        (self.state / "bruno.pending").write_text(self.head + "\nexaminar\ncarlos/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n")
        self.tick()
        self.review.assert_not_called()
        self.db["approved"]["carlos/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"] = True
        self.review.return_value.update(decision="hold", next_action="hold")
        self.tick()
        self.review.assert_called_once()
        self.assertNotIn("grants", self.db)

    def test_budget_and_interrupted_reservation(self):
        self.db["calls"] = list(range(19985, 20001))
        self.tick(20000)
        self.tests.assert_not_called()
        self.review.side_effect = KeyboardInterrupt
        with self.assertRaises(KeyboardInterrupt):
            self.tick(120000)
        self.tick(125000)
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

    def test_hold_empty_proposal_consumed_once(self):
        self.review.return_value["decision"] = "hold"
        self.tick()
        self.assertFalse(self.db["approved"])
        (self.state / "gandalf.json").write_text(g.json.dumps(self.db))
        self.assertEqual(g.consume("bruno", self.root), 0)
        self.assertEqual(g.consume("bruno", self.root), 4)
        self.commit("tools/new")
        self.assertEqual(g.consume("bruno", self.root), 3)

    def test_approval_does_not_authorize_unknown_action(self):
        self.review.return_value["next_action"] = "installer"
        self.tick()
        self.assertNotIn("grants", self.db)

    def test_legacy_migration_preserves_verdict_and_budget(self):
        key = "bruno/" + self.head + "/" + g.hashlib.sha256(b"").hexdigest()
        previous = {"decision": "hold", "release": False, "reason": "dados"}
        self.db["events"][key] = previous.copy()
        self.db["calls"] = [9800]
        with patch.object(g, "LEGACY_SHA", self.head):
            self.tick()
            self.tick()
        self.review.assert_not_called()
        self.assertEqual(self.db["events"][key], previous)
        self.assertEqual(self.db["calls"], [9800])
        self.assertEqual(len(self.db["grants"]), 1)
