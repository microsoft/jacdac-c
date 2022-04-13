const fs = require("fs")
const screenWidth = 120

function decode(fn) {
    const buf = fs.readFileSync(fn)
    const arr = []
    for (let i = 0; i < buf.length; i += 2) {
        const v = buf[i]
        let cnt = buf[i + 1]
        if (cnt > 50) cnt = 50
        while (cnt--)
            arr.push(v)
    }

    return arr
}

function show(arr) {
    let r = ""
    let ch = []

    reset()
    for (const v of arr) {
        for (let i = 0; i < 8; ++i) {
            if (v & (1 << i))
                ch[i] += "#"
            else
                ch[i] += "-"
        }
        if (ch[0].length >= screenWidth) flush()
    }
    flush()

    console.log(r)

    function reset() {
        ch = []
        for (let i = 0; i < 8; ++i)
            ch.push("")
    }

    function flush() {
        for (let i = 0; i < 8; ++i)
            r += ch[i] + "\n"
        r += "\n"
        reset()
    }
}

show(decode(process.argv[2]))
