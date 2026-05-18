const OPCODES = {
  NOP: 0x00, AFC: 0x01, COP: 0x02,
  ADD: 0x03, MUL: 0x04, SOU: 0x05, DIV: 0x06,
  NOT: 0x07, AND: 0x08, OR: 0x09,
  BNOT: 0x0A, BAND: 0x0B, BOR: 0x0C,
  LOAD: 0x0D, STR: 0x0E, LDI: 0x0F, STI: 0x10,
  J: 0x11, JF: 0x12, JI: 0x13,
  LT: 0x14, GT: 0x15, EQ: 0x16, LEQ: 0x17, GEQ: 0x18, NEQ: 0x19,
  PRI: 0x1A
};

const OPCODE_NAMES = Object.fromEntries(Object.entries(OPCODES).map(([name, code]) => [code, name]));
const REGISTER_NAMES = ["r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "sp", "fp", "ra"];
const SPECIAL_REGISTERS = new Set([0, 12, 13, 14, 15]);
const SAMPLE = `0x0000:  AFC r1 0x7
0x0001:  STR 0x0 r1
0x0002:  J 0x6
0x0003:  NOP          % function zero
0x0004:  AFC r0 0x0
0x0005:  JI ra
0x0006:  NOP          % main
0x0007:  AFC sp 0xFF
0x0008:  COP fp sp
0x0009:  LOAD r1 0x0
0x000A:  AFC r2 0x1
0x000B:  ADD r1 r1 r2
0x000C:  PRI r1`;

const state = {
  instructions: [],
  registers: new Array(16).fill(0),
  memory: new Array(256).fill(0),
  previousRegisters: new Array(16).fill(0),
  previousMemory: new Array(256).fill(0),
  pc: 0,
  cycles: 0,
  output: [],
  running: false,
  timer: null,
  halted: false,
  format: "text"
};

const el = {
  input: document.querySelector("#assembly-input"),
  file: document.querySelector("#file-input"),
  loadText: document.querySelector("#load-text-button"),
  sample: document.querySelector("#load-sample-button"),
  step: document.querySelector("#step-button"),
  run: document.querySelector("#run-button"),
  pause: document.querySelector("#pause-button"),
  reset: document.querySelector("#reset-button"),
  speed: document.querySelector("#speed-input"),
  statePill: document.querySelector("#state-pill"),
  pcPill: document.querySelector("#pc-pill"),
  cyclePill: document.querySelector("#cycle-pill"),
  formatPill: document.querySelector("#format-pill"),
  loadMessage: document.querySelector("#load-message"),
  nextInstruction: document.querySelector("#next-instruction"),
  printCount: document.querySelector("#print-count"),
  instructionCount: document.querySelector("#instruction-count"),
  output: document.querySelector("#output-view"),
  instructions: document.querySelector("#instruction-list"),
  registers: document.querySelector("#register-grid"),
  memory: document.querySelector("#memory-grid"),
  pointers: document.querySelector("#pointer-list")
};

function hex(value, width = 2) {
  const normalized = ((value % 256) + 256) % 256;
  return `0x${normalized.toString(16).toUpperCase().padStart(width, "0")}`;
}

function byte(value) {
  return ((value % 256) + 256) % 256;
}

function parseNumber(token) {
  if (!token) return 0;
  const text = token.trim().replace(/^@/, "");
  if (/^-?0x[0-9a-f]+$/i.test(text)) return parseInt(text, 16);
  if (/^-?\d+$/.test(text)) return parseInt(text, 10);
  throw new Error(`Bad number: ${token}`);
}

function parseRegister(token) {
  const text = token.trim().toLowerCase();
  if (text === "_") return 0;
  if (text === "sp") return 13;
  if (text === "fp") return 14;
  if (text === "ra") return 15;
  const match = text.match(/^r(\d+)$/);
  if (!match) throw new Error(`Bad register: ${token}`);
  const reg = Number(match[1]);
  if (reg < 0 || reg > 15) throw new Error(`Register out of range: ${token}`);
  return reg;
}

function cleanLine(line) {
  return line.split("%")[0].split(";")[0].trim();
}

function parseAssembly(text) {
  const instructions = [];
  const lines = text.split(/\r?\n/);

  for (const rawLine of lines) {
    const comment = rawLine.includes("%") ? rawLine.slice(rawLine.indexOf("%") + 1).trim() : "";
    let line = cleanLine(rawLine);
    if (!line) continue;

    const addressMatch = line.match(/^0x[0-9a-f]+:\s*/i);
    if (addressMatch) {
      line = line.slice(addressMatch[0].length).trim();
    }

    const parts = line.split(/\s+/);
    const op = parts[0].toUpperCase();
    if (!(op in OPCODES)) throw new Error(`Unknown instruction: ${op}`);

    const instruction = { op, code: OPCODES[op], a: 0, b: 0, c: 0, text: line, comment };

    if (["ADD", "MUL", "SOU", "DIV", "AND", "OR", "BAND", "BOR", "LT", "GT", "EQ", "LEQ", "GEQ", "NEQ"].includes(op)) {
      instruction.a = parseRegister(parts[1]);
      instruction.b = parseRegister(parts[2]);
      instruction.c = parseRegister(parts[3]);
    } else if (["COP", "NOT", "BNOT", "LDI", "STI"].includes(op)) {
      instruction.a = parseRegister(parts[1]);
      instruction.b = parseRegister(parts[2]);
    } else if (op === "AFC") {
      instruction.a = parseRegister(parts[1]);
      instruction.b = parseNumber(parts[2]);
    } else if (op === "LOAD") {
      instruction.a = parseRegister(parts[1]);
      instruction.b = parseNumber(parts[2]);
    } else if (op === "STR") {
      instruction.a = parseNumber(parts[1]);
      instruction.b = parseRegister(parts[2]);
    } else if (op === "JF") {
      instruction.a = parseRegister(parts[1]);
      instruction.b = parseNumber(parts[2]);
    } else if (op === "J") {
      instruction.a = parseNumber(parts[1]);
    } else if (["JI", "PRI"].includes(op)) {
      instruction.a = parseRegister(parts[1]);
    }

    instructions.push(instruction);
  }

  return instructions;
}

function decodeBinary(buffer) {
  const bytes = new Uint8Array(buffer);
  if (bytes.length % 4 !== 0) {
    throw new Error("Binary length must be a multiple of 4 bytes.");
  }

  const instructions = [];
  for (let i = 0; i < bytes.length; i += 4) {
    const code = bytes[i];
    const op = OPCODE_NAMES[code];
    if (!op) throw new Error(`Unknown opcode byte: ${hex(code)}`);
    const instruction = { op, code, a: bytes[i + 1], b: bytes[i + 2], c: bytes[i + 3], text: "", comment: "" };

    if (op === "J") {
      instruction.a = bytes[i + 2];
      instruction.b = 0;
      instruction.c = 0;
    } else if (op === "JF") {
      instruction.a = bytes[i + 2];
      instruction.b = bytes[i + 3];
      instruction.c = 0;
    } else if (op === "STI") {
      instruction.a = bytes[i + 2];
      instruction.b = bytes[i + 3];
      instruction.c = 0;
    } else if (["JI", "PRI"].includes(op)) {
      instruction.a = bytes[i + 2];
      instruction.b = 0;
      instruction.c = 0;
    }

    instruction.text = formatInstruction(instruction);
    instructions.push(instruction);
  }
  return instructions;
}

function regName(index) {
  return REGISTER_NAMES[index] || `r${index}`;
}

function operandRegister(value) {
  return regName(value);
}

function formatInstruction(instruction) {
  const { op, a, b, c } = instruction;
  if (["ADD", "MUL", "SOU", "DIV", "AND", "OR", "BAND", "BOR", "LT", "GT", "EQ", "LEQ", "GEQ", "NEQ"].includes(op)) {
    return `${op} ${operandRegister(a)} ${operandRegister(b)} ${operandRegister(c)}`;
  }
  if (["COP", "NOT", "BNOT", "LDI", "STI"].includes(op)) return `${op} ${operandRegister(a)} ${operandRegister(b)}`;
  if (op === "AFC") return `${op} ${operandRegister(a)} ${hex(b)}`;
  if (op === "LOAD") return `${op} ${operandRegister(a)} ${hex(b)}`;
  if (op === "STR") return `${op} ${hex(a)} ${operandRegister(b)}`;
  if (op === "JF") return `${op} ${operandRegister(a)} ${hex(b)}`;
  if (op === "J") return `${op} ${hex(a)}`;
  if (["JI", "PRI"].includes(op)) return `${op} ${operandRegister(a)}`;
  return op;
}

function resetMachine(keepMessage = false) {
  pause();
  state.registers.fill(0);
  state.memory.fill(0);
  state.previousRegisters.fill(0);
  state.previousMemory.fill(0);
  state.pc = 0;
  state.cycles = 0;
  state.output = [];
  state.halted = false;
  if (!keepMessage) setMessage("");
  render();
}

function loadProgram(instructions, format) {
  state.instructions = instructions;
  state.format = format;
  resetMachine(true);
  setMessage(`${instructions.length} instruction${instructions.length === 1 ? "" : "s"} loaded`);
}

function setMessage(message, isError = false) {
  el.loadMessage.textContent = message;
  el.loadMessage.className = isError ? "error" : "";
}

function snapshot() {
  state.previousRegisters = state.registers.slice();
  state.previousMemory = state.memory.slice();
}

function jump(address) {
  state.pc = byte(address);
  if (state.pc >= state.instructions.length) {
    state.halted = true;
  }
}

function step() {
  if (state.halted || state.pc < 0 || state.pc >= state.instructions.length) {
    state.halted = true;
    render();
    return;
  }

  snapshot();
  const instruction = state.instructions[state.pc];
  let nextPc = state.pc + 1;
  const r = state.registers;
  const m = state.memory;
  const a = instruction.a;
  const b = instruction.b;
  const c = instruction.c;

  switch (instruction.op) {
    case "NOP": break;
    case "AFC": r[a] = byte(b); break;
    case "COP": r[a] = r[b]; break;
    case "ADD": r[a] = byte(r[b] + r[c]); break;
    case "MUL": r[a] = byte(r[b] * r[c]); break;
    case "SOU": r[a] = byte(r[b] - r[c]); break;
    case "DIV": r[a] = r[c] === 0 ? 0 : byte(Math.trunc(r[b] / r[c])); break;
    case "NOT": r[a] = r[b] === 0 ? 1 : 0; break;
    case "AND": r[a] = r[b] !== 0 && r[c] !== 0 ? 1 : 0; break;
    case "OR": r[a] = r[b] !== 0 || r[c] !== 0 ? 1 : 0; break;
    case "BNOT": r[a] = byte(~r[b]); break;
    case "BAND": r[a] = byte(r[b] & r[c]); break;
    case "BOR": r[a] = byte(r[b] | r[c]); break;
    case "LOAD": r[a] = m[byte(b)]; break;
    case "STR": m[byte(a)] = r[b]; break;
    case "LDI": r[a] = m[byte(r[b])]; break;
    case "STI": m[byte(r[a])] = r[b]; break;
    case "J": nextPc = byte(a); break;
    case "JF": if (r[a] === 0) nextPc = byte(b); break;
    case "JI": nextPc = byte(r[a]); break;
    case "LT": r[a] = r[b] < r[c] ? 1 : 0; break;
    case "GT": r[a] = r[b] > r[c] ? 1 : 0; break;
    case "EQ": r[a] = r[b] === r[c] ? 1 : 0; break;
    case "LEQ": r[a] = r[b] <= r[c] ? 1 : 0; break;
    case "GEQ": r[a] = r[b] >= r[c] ? 1 : 0; break;
    case "NEQ": r[a] = r[b] !== r[c] ? 1 : 0; break;
    case "PRI": state.output.push(String(r[a])); break;
  }

  state.pc = nextPc;
  state.cycles += 1;
  if (state.pc >= state.instructions.length) state.halted = true;
  render();
}

function run() {
  if (state.running) return;
  state.running = true;
  const tick = () => {
    if (!state.running) return;
    if (state.halted) {
      pause();
      render();
      return;
    }
    step();
    state.timer = window.setTimeout(tick, Number(el.speed.value));
  };
  tick();
  render();
}

function pause() {
  state.running = false;
  if (state.timer) {
    window.clearTimeout(state.timer);
    state.timer = null;
  }
}

function pointerNotesForRegister(index, value) {
  if (index === 0) return "return value";
  if (index === 12) return "temporary";
  if (index === 13) return "stack pointer";
  if (index === 14) return "frame pointer";
  if (index === 15) return "return address";
  return "";
}

function renderRegisters() {
  el.registers.innerHTML = "";
  state.registers.forEach((value, index) => {
    const cell = document.createElement("div");
    cell.className = "register-cell";
    if (SPECIAL_REGISTERS.has(index)) cell.classList.add("special");
    if (value !== state.previousRegisters[index]) cell.classList.add("changed");
    cell.innerHTML = `
      <div class="register-name">${regName(index)}</div>
      <div class="register-value">${value}</div>
      <div class="register-hex">${hex(value)}</div>
      <div class="register-note">${pointerNotesForRegister(index, value)}</div>
    `;
    el.registers.appendChild(cell);
  });
}

function renderMemory() {
  const stackAddress = byte(state.registers[13]);
  const frameAddress = byte(state.registers[14]);
  const lowStart = 0;
  const highStart = state.memory.length - 32;

  const makeMemoryRow = (address) => {
    const markers = [];
    if (address === stackAddress) markers.push("sp");
    if (address === frameAddress) markers.push("fp");

    const row = document.createElement("div");
    row.className = "memory-row";
    if (state.memory[address] !== state.previousMemory[address]) row.classList.add("changed");
    if (markers.length > 0) row.classList.add("pointed");
    row.innerHTML = `
      <span class="memory-marker">${markers.length > 0 ? `${markers.join("/")} ->` : ""}</span>
      <span class="memory-address">${hex(address)}</span>
      <span class="memory-value">${hex(state.memory[address])}</span>
    `;
    return row;
  };

  el.memory.innerHTML = "";

  const lowColumn = document.createElement("div");
  lowColumn.className = "memory-column";
  lowColumn.innerHTML = `<div class="memory-column-title">low</div>`;
  for (let address = lowStart; address < lowStart + 32; address += 1) {
    lowColumn.appendChild(makeMemoryRow(address));
  }

  const highColumn = document.createElement("div");
  highColumn.className = "memory-column";
  highColumn.innerHTML = `<div class="memory-column-title">high</div>`;
  for (let address = state.memory.length - 1; address >= highStart; address -= 1) {
    highColumn.appendChild(makeMemoryRow(address));
  }

  el.memory.appendChild(lowColumn);
  el.memory.appendChild(highColumn);
}

function renderPointers() {
  const stackAddress = byte(state.registers[13]);
  const frameAddress = byte(state.registers[14]);
  const returnAddress = byte(state.registers[15]);
  const items = [
    ["pc", state.pc, state.instructions[state.pc] ? formatInstruction(state.instructions[state.pc]) : "halt"],
    ["sp", state.registers[13], `memory ${hex(stackAddress)} = ${hex(state.memory[stackAddress])}`],
    ["fp", state.registers[14], `memory ${hex(frameAddress)} = ${hex(state.memory[frameAddress])}`],
    ["ra", state.registers[15], `instruction ${hex(returnAddress)}`],
    ["r0", state.registers[0], "return value"]
  ];

  el.pointers.innerHTML = "";
  for (const [name, value, note] of items) {
    const row = document.createElement("div");
    row.className = "pointer-item";
    row.innerHTML = `<code>${name} ${value} (${hex(value)})</code><span>${note}</span>`;
    el.pointers.appendChild(row);
  }
}

function renderInstructions() {
  el.instructions.innerHTML = "";
  state.instructions.forEach((instruction, index) => {
    const row = document.createElement("div");
    row.className = "instruction-row";
    if (index === state.pc && !state.halted) row.classList.add("active");
    if (index < state.pc) row.classList.add("executed");
    const comment = instruction.comment ? `<span class="instruction-comment"> % ${instruction.comment}</span>` : "";
    row.innerHTML = `
      <div class="instruction-address">${hex(index, 4)}</div>
      <div>${formatInstruction(instruction)}${comment}</div>
    `;
    el.instructions.appendChild(row);
  });
}

function renderStatus() {
  el.statePill.textContent = state.running ? "running" : state.halted ? "halted" : "ready";
  el.pcPill.textContent = `pc ${hex(state.pc)}`;
  el.cyclePill.textContent = `cycle ${state.cycles}`;
  el.nextInstruction.textContent = state.instructions[state.pc] && !state.halted ? formatInstruction(state.instructions[state.pc]) : "halt";
  el.printCount.textContent = state.output.length;
  el.instructionCount.textContent = state.instructions.length;
  el.output.textContent = state.output.join("\n");
  el.formatPill.textContent = state.format;
}

function render() {
  renderStatus();
  renderInstructions();
  renderRegisters();
  renderMemory();
  renderPointers();
}

function loadTextFromEditor() {
  try {
    loadProgram(parseAssembly(el.input.value), "text");
  } catch (error) {
    setMessage(error.message, true);
  }
}

function loadSample() {
  el.input.value = SAMPLE;
  loadTextFromEditor();
}

el.loadText.addEventListener("click", loadTextFromEditor);
el.sample.addEventListener("click", loadSample);
el.step.addEventListener("click", step);
el.run.addEventListener("click", run);
el.pause.addEventListener("click", () => {
  pause();
  render();
});
el.reset.addEventListener("click", () => resetMachine());

el.file.addEventListener("change", async (event) => {
  const file = event.target.files[0];
  if (!file) return;

  try {
    if (file.name.toLowerCase().endsWith(".bin")) {
      const buffer = await file.arrayBuffer();
      loadProgram(decodeBinary(buffer), "binary");
      el.input.value = state.instructions.map((instruction, index) => `${hex(index, 4)}:  ${formatInstruction(instruction)}`).join("\n");
    } else {
      const text = await file.text();
      el.input.value = text;
      loadProgram(parseAssembly(text), "text");
    }
  } catch (error) {
    setMessage(error.message, true);
  } finally {
    event.target.value = "";
  }
});

loadSample();
