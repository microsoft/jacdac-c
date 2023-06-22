let fs = require("fs")
let sums = {}
let inprog = false
let inram = false
let indata = false
let fun = ""
let dofile = process.argv[3] == "-file" 
let dofun = process.argv[3] == "-fun"
let funline = ""
for (let line of fs.readFileSync(process.argv[2], "utf8").split(/\r?\n/)) {
  if (/\*fill\*/.test(line)) continue
  if (/^r[oa]m\s/.test(line)) continue
  let m = /^ \.(\w+)/.exec(line)
  if (m) {
    if (m[1] == "text" || m[1] == "binmeta" || m[1] == "rodata" || m[1] == "data" || m[1] == "isr_vector" || m[1] == "iram1")
      inprog = true
    else
      inprog = false
    if (m[1] == "data" || m[1] == "bss" || m[1] == "sbss" || m[1] == "noinit")
      inram = true
    else
      inram = false
    if (m[1] == "data" || m[1] == "rodata")
      indata = true
    else
      indata = false
    fun = line.slice(m[1].length + 2).replace(/\s.*/, "")
    funline = line
  }
  if (!inprog && !inram) continue
  m = /\s+(0x00[a-f0-9]+)\s+(0x[a-f0-9]+)\s+(.*)/.exec(line)
  if (!m) continue
  const iniram = /^0x00000000403/.test(m[1])

  let addr = parseInt(m[1])
  let size = parseInt(m[2])
  if (!addr || !size) continue
  let name = m[3]
  if (/load address/.test(name)) continue
  if (name == "xr" || name == "xrw") continue
  name = name.replace(/.*\/lib/, "lib")
  name = name.replace(/.*__\//, "")
  if(!dofile && !dofun)
    name=name.replace(/\(.*/, "")
  if (dofun) name += fun

  function doSum(pref) {
    const n = pref + "." + name
    if (!sums[n]) sums[n] = 0
    sums[n] += size
    if (!sums[pref + ".TOTAL"]) sums[pref + ".TOTAL"] = 0
    sums[pref + ".TOTAL"] += size
  }

  if (indata)
    doSum("DATA")
  else if (inprog)
    doSum("TEXT")

  if (inprog)
    doSum("FLASH")

  if (inram)
    doSum("RAM")

  if (iniram)
    doSum("IRAM")

  let pref = inram ? "RAM." : ""

  //if (!inram && size > 100) console.log(line)

  name = pref + name

  //console.log(name, size, line)

}

function order(pref) {
  const k = Object.keys(sums).filter(s => s.startsWith(pref + "."))
  k.sort((a, b) => sums[a] - sums[b])
  for (let kk of k) {
    console.log(kk, sums[kk])
  }
  console.log("\n")
}

order("RAM")
order("DATA")
order("TEXT")
order("FLASH")
order("IRAM")

