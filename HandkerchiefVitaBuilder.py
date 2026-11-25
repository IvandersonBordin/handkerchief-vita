#!/usr/bin/env python3
"""Entry point of the packaged builder (double-click window). With command-line arguments it behaves like build.py."""
import multiprocessing, sys

if __name__ == "__main__":
    multiprocessing.freeze_support()
    if len(sys.argv) > 1:
        import build
        build.main()
    else:
        from hkbuild import gui
        sys.exit(gui.main())
