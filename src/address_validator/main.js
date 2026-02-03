(async function () {
  "use strict";

  const $ = (id) => document.getElementById(id);

  const out = $("out");
  const addrInput = $("addr");
  const btnCheck = $("btnCheck");
  const btnClear = $("btnClear");

  const write = (msg) => {
    out.textContent = String(msg);
  };

  const codeToText = (code) => {
    switch (code) {
      case 0: return "ok (valid, no payment id)";
      case 1: return "invalid";
      case 2: return "bad_args (null pointer)";
      case 3: return "zano_integrated_ok (valid + payment id)";
      case 4: return "wrapped_like_ok (looks like wrapped address)";
      default: return `unknown (${code})`;
    }
  };

  try {
    write("Loading WASM module…");

    if (typeof ZanoAddrValidator !== "function") {
      throw new Error("ZanoAddrValidator is not defined. Check script order in index.html.");
    }

    const Module = await ZanoAddrValidator({
      print: (txt) => console.log("[wasm]", txt),
      printErr: (txt) => console.error("[wasm-err]", txt),
    });

    if (typeof Module.cwrap !== "function") {
      throw new Error("Module.cwrap is missing. Check -sEXPORTED_RUNTIME_METHODS.");
    }

    // C signature: uint8_t zano_validate_address(const char* addr)
    const validate = Module.cwrap("zano_validate_address", "number", ["string"]);

    const runCheck = () => {
      const addr = (addrInput.value || "").trim();
      if (!addr) {
        write("Paste an address first.");
        return;
      }

      const code = validate(addr);
      const text = codeToText(code);

      write(
        [
          `Address: ${addr}`,
          `Code: ${code}`,
          `Meaning: ${text}`,
        ].join("\n")
      );
    };

    btnCheck.addEventListener("click", runCheck);
    btnClear.addEventListener("click", () => {
      addrInput.value = "";
      write("");
      addrInput.focus();
    });

    addrInput.addEventListener("keydown", (e) => {
      if (e.key === "Enter") runCheck();
    });

    write("Ready. Paste an address and press Check.");
    addrInput.focus();
  } catch (e) {
    console.error(e);
    write("ERROR:\n" + (e && e.stack ? e.stack : String(e)));
  }
})();
