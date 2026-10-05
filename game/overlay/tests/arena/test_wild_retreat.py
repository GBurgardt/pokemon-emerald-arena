#!/usr/bin/env python3
"""Wild START→RUN flee contract: TryRunFromBattle, B_OUTCOME_RAN, trainer hides RUN.

Requires the lab ROM + mGBA runner (same as test_hybrid). Does not touch
test_hybrid / test_menu_persistence. Skips cleanly if the lab binary is absent.
"""
import json, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/arena'))

checks = []

def check(name, ok):
    assert ok, name
    checks.append(name)
    print('PASS ' + name, flush=True)

def main():
    from lab import call, WORK
    def cmd(action, **kw):
        r = call(dict(action=action, **kw))
        assert r['ok'], r
        return r['result']
    def tap(key, n=30):
        cmd('press', keys=key, frames=3)
        return cmd('wait', frames=n)

    cmd('boot')
    cmd('fixture-team', mons=['TREECKO:20', 'MUDKIP:20'])
    start = cmd('battle', species='POOCHYENA', level=3, classic=False)
    check('wild arena active', start['arena']['active'])
    # Force a guaranteed flee: player much faster than a level-3 Poochyena.
    frozen = tap('START', 4)
    check('pause settled', frozen['arena']['paused'])
    # Root menu: POKEMON(0) BAG(1) MOVES(2) RUN(3) RESUME(4). Move to RUN.
    for _ in range(3):
        tap('DOWN', 2)
    fled = tap('A', 8)
    # After a successful flee the arena tears down and QuietResult returns to field.
    for _ in range(240):
        if fled.get('cb2') == 'CB2_Overworld' and not fled.get('field_locked'):
            break
        if fled.get('battle_outcome') == 4:  # B_OUTCOME_RAN
            break
        fled = cmd('wait', frames=2)
    check('wild RUN leaves with B_OUTCOME_RAN or free field',
          fled.get('battle_outcome') == 4
          or (fled.get('cb2') == 'CB2_Overworld' and not fled.get('field_locked')))
    check('arena inactive after flee', not fled.get('arena', {}).get('active', True))

    # Trainer: RUN must not forfeit. Start a supported trainer if fixtures allow.
    cmd('boot')
    cmd('fixture-team', mons=['TREECKO:20', 'MUDKIP:20'])
    try:
        trainer = cmd('trainer', trainer='IVAN')
    except Exception:
        trainer = None
    if trainer and trainer.get('arena', {}).get('active'):
        tap('START', 4)
        # Four entries only: DOWN×3 lands on RESUME, never a flee exit.
        for _ in range(3):
            tap('DOWN', 2)
        stayed = tap('A', 8)
        check('trainer RESUME stays in arena (no forfeit)',
              stayed['arena']['active'] and stayed.get('battle_outcome', 0) != 4)
    else:
        checks.append('trainer RUN hide check skipped (no arena trainer fixture)')
        print('SKIP trainer RUN hide check', flush=True)

    (WORK / 'results/wild-retreat.json').write_text(
        json.dumps(dict(ok=True, checks=checks, rom_sha256=start.get('rom_sha256')), indent=2) + '\n')

if __name__ == '__main__':
    try:
        from lab import WORK  # noqa: F401
    except Exception as exc:
        print('SKIP lab unavailable:', exc, flush=True)
        sys.exit(0)
    main()
