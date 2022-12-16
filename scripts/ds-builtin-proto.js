const fs = require("fs")
const path = require("path")

const scriptArgs = process.argv.slice(2)
let numerr = 0

const byObj = {}
const allfuns = []

const firstFun = 0xf000

let r = `// auto-generated!
#include "devs_internal.h"

#define PROP DEVS_BUILTIN_FLAG_IS_PROPERTY
#define ASYNC DEVS_BUILTIN_FLAG_ASYNC_CALL 0x02
#define NO_SELF DEVS_BUILTIN_FLAG_NO_SELF 0x04

#define N(n) (DEVS_BUILTIN_STRING_ ## n)

`

let maxParms = 0
for (const fn of scriptArgs) {
    let lineNo = 0
    r += `// ${path.basename(fn)}\n`
    for (const ln of fs.readFileSync(fn, "utf-8").split(/\r?\n/)) {
        ++lineNo
        const m = /^(\w+) ((fun|prop)_([a-zA-Z0-9]+)_(\w+))\((.*)\)/.exec(ln)
        if (!m)
            continue
        const [full, retTp, fnName, funProp, className, methodName, argString] = m
        const flags = []
        let objId = className

        r += `${full};\n`

        const argWords = argString.split(/,\s*/).map(s => s.trim())
        if (argWords[0] != "devs_ctx_t *ctx")
            error("first arg should be ctx")
        if (retTp == "value_t") {
            // OK
        } else if (retTp == "void") {
            flags.push("ASYNC")
        } else {
            error("only void and value_t supported as return")
        }
        if (funProp == "prop")
            flags.push("PROP")
        const params = argWords.slice(1)
        if (params[0] == "value_t self") {
            params.shift()
            objId += "_prototype"
        } else {
            flags.push("NO_SELF")
        }
        for (const p of params) {
            if (/^value_t\s*\w+$/.test(p)) {
                // OK
            } else {
                error(`invalid param ${p}`)
            }
        }
        maxParms = Math.max(params.length, maxParms)

        const fl = flags.length == 0 ? "0" : flags.join("|")

        if (!byObj[objId])
            byObj[objId] = []
        byObj[objId].push(`{ N(${methodName.toUpperCase()}), 0x${(firstFun + allfuns.length).toString(16)} }`)

        allfuns.push(`{ N(${methodName.toUpperCase()}), ${params.length}, ${fl}, (void*)${fnName}  }`)
    }

    function error(msg) {
        console.error(`${fn}:${lineNo}: ${msg}`)
        numerr++
    }
}

if (numerr)
    process.exit(1)

r += "\n"
for (const k of Object.keys(byObj)) {
    r += `static const devs_builtin_proto_entry_t ${k}_entries[] = { //\n`
    r += byObj[k].join(",\n")
    r += ",\n{ 0, 0 }};\n\n"
}

r += `const devs_builtin_proto_t devs_builtin_protos[DEVS_BUILTIN_OBJECT___MAX + 1] = {\n`
for (const k of Object.keys(byObj)) {
    r += `[DEVS_BUILTIN_OBJECT_${k.toUpperCase()}] = { DEVS_BUILTIN_PROTO_INIT, ${k}_entries },\n`
}

r += "};\n\n"


r += `uint16_t devs_num_builtin_functions = ${allfuns.length};\n`
r += `const devs_builtin_function_t devs_builtin_functions[${allfuns.length}] = {\n`
r += allfuns.join(",\n")
r += "};\n\n"

r += `STATIC_ASSERT(${maxParms} <= DEVS_BUILTIN_MAX_ARGS);\n`
r += `STATIC_ASSERT(${firstFun} == DEVS_FIRST_BUILTIN_FUNCTION);\n`

fs.writeFileSync(path.dirname(scriptArgs[0]) + "/protogen.c", r)