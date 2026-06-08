'use strict';

// ── Constants ────────────────────────────────────────────────────────────────
const BACKEND = window.BACKEND_URL || '';  // injected by nginx or empty for same-origin
const P0_KALAHA = 6;
const P1_KALAHA = 13;

// ── State ────────────────────────────────────────────────────────────────────
let state = {
  board:   Array(14).fill(4),
  side:    0,
  over:    false,
  busy:    false,
};

// ── Init board ───────────────────────────────────────────────────────────────
function initBoard() {
  const b = Array(14).fill(4);
  b[P0_KALAHA] = 0;
  b[P1_KALAHA] = 0;
  return b;
}

// ── Render ───────────────────────────────────────────────────────────────────
function render() {
  const board = state.board;

  document.getElementById('k0-count').textContent = board[P0_KALAHA];
  document.getElementById('k1-count').textContent = board[P1_KALAHA];

  const humanSide = parseInt(document.getElementById('human-side').value);
  const isHumanTurn = state.side === humanSide && !state.over && !state.busy;

  // P0 row: pits 0..5 left to right
  const rowP0 = document.getElementById('row-p0');
  rowP0.innerHTML = '';
  for (let i = 0; i < 6; i++) {
    rowP0.appendChild(makePit(i, board[i], state.side === 0 && isHumanTurn && board[i] > 0));
  }

  // P1 row: pits 12..7 (displayed right-to-left from P1's perspective, but left-to-right visually facing P0)
  const rowP1 = document.getElementById('row-p1');
  rowP1.innerHTML = '';
  for (let i = 12; i >= 7; i--) {
    rowP1.appendChild(makePit(i, board[i], state.side === 1 && isHumanTurn && board[i] > 0));
  }
}

function makePit(index, count, clickable) {
  const div = document.createElement('div');
  div.className = 'pit' + (count === 0 ? ' empty' : '') + (clickable ? ' clickable' : '');
  div.textContent = count;
  const label = document.createElement('span');
  label.className = 'pit-label';
  label.textContent = `p${index}`;
  div.appendChild(label);
  if (clickable) div.onclick = () => humanMove(index);
  return div;
}

// ── Status helpers ───────────────────────────────────────────────────────────
function setStatus(msg, cls = '') {
  const el = document.getElementById('status');
  el.textContent = msg;
  el.className = cls;
}
function setAIInfo(msg) {
  document.getElementById('ai-info').textContent = msg;
}

// ── New game ─────────────────────────────────────────────────────────────────
function newGame() {
  state = { board: initBoard(), side: 0, over: false, busy: false };
  setAIInfo('');
  setStatus('Turno de P0');
  render();
  maybeAIMove();
}

// ── Human move ───────────────────────────────────────────────────────────────
function humanMove(pit) {
  if (state.busy || state.over) return;
  const humanSide = parseInt(document.getElementById('human-side').value);
  if (state.side !== humanSide) return;

  applyMoveLocally(pit);
  render();
  checkTerminal();
  if (!state.over) maybeAIMove();
}

// ── Local board logic (mirrors C++ rules for immediate feedback) ──────────────
function applyMoveLocally(pit) {
  const b     = state.board.slice();
  const side  = state.side;
  const oppK  = side === 0 ? P1_KALAHA : P0_KALAHA;
  const ownK  = side === 0 ? P0_KALAHA : P1_KALAHA;

  let seeds = b[pit];
  b[pit] = 0;
  let pos = pit;

  while (seeds > 0) {
    pos = (pos + 1) % 14;
    if (pos === oppK) continue;
    b[pos]++;
    seeds--;
  }

  let extraTurn = false;

  if (pos === ownK) {
    extraTurn = true;
  } else {
    // Capture
    const ownStart = side === 0 ? 0 : 7;
    const ownEnd   = side === 0 ? 5 : 12;
    if (pos >= ownStart && pos <= ownEnd && b[pos] === 1) {
      const opp = 12 - pos;
      if (b[opp] > 0) {
        b[ownK] += b[pos] + b[opp];
        b[pos] = 0;
        b[opp] = 0;
      }
    }
  }

  state.board = b;
  if (!extraTurn) state.side = 1 - side;
  setStatus(extraTurn ? `P${side} juega de nuevo (turno extra)` : `Turno de P${state.side}`);
}

function checkTerminal() {
  const b = state.board;
  const sum0 = b.slice(0, 6).reduce((a, v) => a + v, 0);
  const sum1 = b.slice(7, 13).reduce((a, v) => a + v, 0);
  if (sum0 === 0 || sum1 === 0) {
    // Collect remaining
    const bc = b.slice();
    for (let i = 0; i < 6; i++) { bc[P0_KALAHA] += bc[i]; bc[i] = 0; }
    for (let i = 7; i <= 12; i++) { bc[P1_KALAHA] += bc[i]; bc[i] = 0; }
    state.board = bc;
    state.over  = true;
    const k0 = bc[P0_KALAHA], k1 = bc[P1_KALAHA];
    let msg = k0 > k1 ? `¡P0 gana! (${k0} vs ${k1})`
            : k1 > k0 ? `¡P1 gana! (${k1} vs ${k0})`
            : `¡Empate! (${k0} - ${k1})`;
    setStatus(msg, 'winner');
    render();
  }
}

// ── AI move (via backend) ─────────────────────────────────────────────────────
async function maybeAIMove() {
  const humanSide = parseInt(document.getElementById('human-side').value);
  if (state.over || state.busy || state.side === humanSide) return;

  state.busy = true;
  setStatus(`IA pensando (P${state.side})…`, 'thinking');
  render();

  const depth   = parseInt(document.getElementById('depth').value)   || 8;
  const threads = parseInt(document.getElementById('threads').value) || 4;

  try {
    const resp = await fetch(`${BACKEND}/move`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        board:   state.board,
        side:    state.side,
        depth,
        threads,
      }),
    });

    if (!resp.ok) {
      const err = await resp.json().catch(() => ({ error: resp.statusText }));
      throw new Error(err.error || resp.statusText);
    }

    const data = await resp.json();
    setAIInfo(
      `Jugada: p${data.move} | Eval: ${data.evaluation} | ` +
      `Tiempo: ${data.elapsed_ms}ms | Nodos: ${data.stats.nodes.toLocaleString()} | ` +
      `Podas: ${data.stats.prunes.toLocaleString()} | Hilos: ${data.threads_used}`
    );

    state.busy = false;
    applyMoveLocally(data.move);
    render();
    checkTerminal();
    if (!state.over) maybeAIMove();

  } catch (e) {
    state.busy = false;
    setStatus(`Error de IA: ${e.message}`, 'error');
    render();
  }
}

// ── Boot ──────────────────────────────────────────────────────────────────────
window.newGame = newGame;
