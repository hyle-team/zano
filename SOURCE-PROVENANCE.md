# Mobile SDK source snapshot

This commit starts the maintained mobile SDK source history on branch `feat/mobile-publishable-snapshot-2026-11-11`. It is a root commit with no parent.

Its tree is the maintained mobile SDK source from local Zano commit e90a4e181fff7ff96ce03ddeda565d07cb5f75e8, whose upstream reference is hyle-team/zano commit ee3de1e5a077b60106ba88301e236474680b1028. Every retained file matches that local commit exactly in content and mode, including source, licensing and build definitions, and the four submodule gitlinks point to the same commits. This note is the only added file. Three legacy examples are omitted:

- utils/JS/JWT/example.js
- utils/JS/test_eth_sig.js
- utils/JS/emmit_response.json

Omitting them does not erase or rotate any value in existing public copies.

The original local branches and their history are retained unchanged. The two commits named above are historical source and ancestry references only. This root does not carry their commit ancestry, and it does not reproduce or prove how existing SDK binaries were built.

A snapshot-aware library profile must bind this commit's actual commit id, tree, file census and submodule commits, and derive a normal new SDK version from this history's Git metadata. The previously observed 308-file input set and archive evidence stay a separate record and do not describe anything built from this snapshot.

This is a source-preservation snapshot. It is not a published SDK and makes no build, runtime or platform qualification claim.
