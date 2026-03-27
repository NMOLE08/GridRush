const { useMemo, useState, useEffect } = React;

const CATEGORIES = [
  { id: 'classic', title: 'Classic Sudoku', subtitle: 'Row / Column / Box constraints' },
  { id: 'even-odd', title: 'Even/Odd Sudoku', subtitle: 'Parity markers + classic rules' },
  { id: 'killer', title: 'Killer Sudoku', subtitle: 'Cage sums + no repeats in cage' },
  { id: 'thermo', title: 'Thermo Sudoku', subtitle: 'Strictly increasing along thermometers' },
  { id: 'arrow', title: 'Arrow Sudoku', subtitle: 'Arrow path sums to circle cell' },
  { id: 'kropki', title: 'Kropki Sudoku', subtitle: 'Consecutive and doubling dots' },
  { id: 'hybrid', title: 'Hybrid', subtitle: 'Mix multiple variants together' },
];

const VARIANT_TEMPLATES = {
  classic: {},
  'even-odd': {
    even_cells: [[1, 2], [2, 5]],
    odd_cells: [[1, 1], [3, 3]],
  },
  killer: {
    killer_cages: [
      { sum: 10, cells: [[1, 1], [1, 2]] },
      { sum: 15, cells: [[1, 3], [2, 3], [2, 2]] }
    ]
  },
  thermo: {
    thermos: [
      { cells: [[1, 1], [1, 2], [1, 3], [2, 3]] }
    ]
  },
  arrow: {
    arrows: [
      { circle: [2, 2], path: [[2, 3], [2, 4], [3, 4]] }
    ]
  },
  kropki: {
    kropki: [
      { a: [1, 1], b: [1, 2], type: 'white' },
      { a: [2, 2], b: [2, 3], type: 'black' }
    ]
  },
  hybrid: {
    even_cells: [[1, 2]],
    thermos: [
      { cells: [[1, 1], [1, 2], [1, 3]] }
    ],
    kropki: [
      { a: [3, 3], b: [3, 4], type: 'white' }
    ]
  }
};

function emptyGrid() {
  return Array.from({ length: 9 }, () => Array(9).fill(0));
}

function cellBorderClasses(r, c) {
  return `${(c + 1) % 3 === 0 && c !== 8 ? 'box-right' : ''} ${(r + 1) % 3 === 0 && r !== 8 ? 'box-bottom' : ''}`.trim();
}

function formatCell(cell) {
  if (!Array.isArray(cell) || cell.length < 2) return '(invalid)';
  return `R${cell[0]}C${cell[1]}`;
}

function summarizeVariantConstraints(variantData = {}) {
  const lines = [];

  if (Array.isArray(variantData.even_cells) && variantData.even_cells.length) {
    lines.push(`Even cells: ${variantData.even_cells.map(formatCell).join(', ')}`);
  }
  if (Array.isArray(variantData.odd_cells) && variantData.odd_cells.length) {
    lines.push(`Odd cells: ${variantData.odd_cells.map(formatCell).join(', ')}`);
  }
  if (Array.isArray(variantData.killer_cages) && variantData.killer_cages.length) {
    lines.push(`Killer cages: ${variantData.killer_cages.length}`);
  }
  if (Array.isArray(variantData.thermos) && variantData.thermos.length) {
    lines.push(`Thermometers: ${variantData.thermos.length}`);
  }
  if (Array.isArray(variantData.arrows) && variantData.arrows.length) {
    lines.push(`Arrows: ${variantData.arrows.length}`);
  }
  if (Array.isArray(variantData.kropki) && variantData.kropki.length) {
    lines.push(`Kropki pairs: ${variantData.kropki.length}`);
  }

  return lines;
}

function getParityType(r, c, variantData) {
  if (!variantData) return '';
  const targetRow = r + 1;
  const targetCol = c + 1;

  const isEven = Array.isArray(variantData.even_cells)
    && variantData.even_cells.some((cell) => Array.isArray(cell) && Number(cell[0]) === targetRow && Number(cell[1]) === targetCol);
  if (isEven) return 'even';

  const isOdd = Array.isArray(variantData.odd_cells)
    && variantData.odd_cells.some((cell) => Array.isArray(cell) && Number(cell[0]) === targetRow && Number(cell[1]) === targetCol);
  if (isOdd) return 'odd';

  return '';
}

function App() {
  const [category, setCategory] = useState(null);
  const [grid, setGrid] = useState(emptyGrid());
  const [variantText, setVariantText] = useState('{}');
  const [markerMode, setMarkerMode] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [solverFound, setSolverFound] = useState(true);
  const [result, setResult] = useState(null);

  useEffect(() => {
    fetch('/api/health')
      .then((r) => r.json())
      .then((d) => setSolverFound(Boolean(d.solverFound)))
      .catch(() => setSolverFound(false));
  }, []);

  useEffect(() => {
    if (!category) return;
    const template = VARIANT_TEMPLATES[category] || {};
    setVariantText(JSON.stringify(template));
    setMarkerMode('');
    setResult(null);
    setError('');
  }, [category]);

  const canSolve = useMemo(() => Boolean(category) && !loading, [category, loading]);
  const parsedVariant = useMemo(() => {
    try {
      const parsed = variantText.trim() ? JSON.parse(variantText) : {};
      return { data: parsed, error: '' };
    } catch {
      return { data: null, error: 'Invalid JSON format.' };
    }
  }, [variantText]);
  const variantSummary = useMemo(
    () => summarizeVariantConstraints(parsedVariant.data || {}),
    [parsedVariant]
  );
  const supportsParityMarkers = useMemo(
    () => category === 'even-odd' || category === 'hybrid',
    [category]
  );

  function updateCell(r, c, value) {
    const next = grid.map((row) => row.slice());
    if (value === '') {
      next[r][c] = 0;
    } else {
      const num = Number(value);
      if (!Number.isInteger(num) || num < 1 || num > 9) return;
      next[r][c] = num;
    }
    setGrid(next);
  }

  function clearGrid() {
    setGrid(emptyGrid());
    setResult(null);
    setError('');
  }

  async function solvePuzzle() {
    setLoading(true);
    setError('');
    setResult(null);

    let variantData = {};
    try {
      variantData = variantText.trim() ? JSON.parse(variantText) : {};
    } catch {
      setLoading(false);
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    try {
      const res = await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          category,
          givensGrid: grid,
          variantData,
        }),
      });

      const data = await res.json();
      if (!res.ok || !data.ok) {
        throw new Error(data.details || data.error || 'Failed to solve puzzle.');
      }

      setResult(data);
    } catch (e) {
      setError(e.message || 'Unknown error while solving.');
    } finally {
      setLoading(false);
    }
  }

  function compactVariantJson() {
    try {
      const parsed = variantText.trim() ? JSON.parse(variantText) : {};
      setVariantText(JSON.stringify(parsed));
    } catch {
      setError('Variant JSON is invalid. Fix it and try again.');
    }
  }

  function toggleMarkerMode(mode) {
    setMarkerMode((current) => (current === mode ? '' : mode));
  }

  function updateParityMarker(r, c, mode) {
    if (!supportsParityMarkers || !mode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    const next = { ...parsedVariant.data };
    const row = r + 1;
    const col = c + 1;
    const currentType = getParityType(r, c, next);

    const normalize = (cells) => (
      Array.isArray(cells)
        ? cells.filter((cell) => Array.isArray(cell) && cell.length >= 2)
        : []
    );

    const withoutTarget = (cells) => cells.filter(
      (cell) => !(Number(cell[0]) === row && Number(cell[1]) === col)
    );

    let evenCells = withoutTarget(normalize(next.even_cells));
    let oddCells = withoutTarget(normalize(next.odd_cells));

    if (currentType !== mode) {
      if (mode === 'even') {
        evenCells = [...evenCells, [row, col]];
      } else if (mode === 'odd') {
        oddCells = [...oddCells, [row, col]];
      }
    }

    next.even_cells = evenCells;
    next.odd_cells = oddCells;

    setVariantText(JSON.stringify(next));
    setError('');
  }

  if (!category) {
    return (
      <div className="page">
        <header className="header">
          <h1>GridRush Sudoku Solver</h1>
          <p>Select a puzzle category first.</p>
        </header>

        <section className="category-grid">
          {CATEGORIES.map((cat) => (
            <button key={cat.id} className="card" onClick={() => setCategory(cat.id)}>
              <h3>{cat.title}</h3>
              <p>{cat.subtitle}</p>
            </button>
          ))}
        </section>

        {!solverFound && (
          <div className="warning">
            Solver binary was not found. Build backend first.
          </div>
        )}
      </div>
    );
  }

  return (
    <div className="page">
      <header className="header row">
        <div>
          <h1>Category: {CATEGORIES.find((c) => c.id === category)?.title}</h1>
          <p>Fill the Sudoku grid, adjust variant constraints JSON, then click Start Solver.</p>
        </div>
        <button className="secondary" onClick={() => setCategory(null)}>Change Category</button>
      </header>

      <main className={`layout ${category === 'even-odd' ? 'single-column' : ''}`}>
        <section className="panel">
          <h2>Input Grid</h2>
          {supportsParityMarkers && (
            <div className="marker-tools">
              <p className="hint">Marker Tool: choose a shape, then click cells to place/remove it.</p>
              <div className="marker-buttons">
                <button
                  className={`secondary marker-toggle ${markerMode === 'even' ? 'active' : ''}`}
                  onClick={() => toggleMarkerMode('even')}
                  type="button"
                >
                  <span className="tool-icon even" aria-hidden="true"></span>
                  Square
                </button>
                <button
                  className={`secondary marker-toggle ${markerMode === 'odd' ? 'active' : ''}`}
                  onClick={() => toggleMarkerMode('odd')}
                  type="button"
                >
                  <span className="tool-icon odd" aria-hidden="true"></span>
                  Circle
                </button>
              </div>
            </div>
          )}
          <div className="sudoku-grid">
            {grid.map((row, r) =>
              row.map((cell, c) => {
                const parityType = getParityType(r, c, parsedVariant.data);
                return (
                  <div
                    key={`${r}-${c}`}
                    className={`grid-cell ${cellBorderClasses(r, c)} ${markerMode ? 'marker-mode' : ''}`}
                    onMouseDown={(e) => {
                      if (!markerMode || !supportsParityMarkers) return;
                      e.preventDefault();
                      updateParityMarker(r, c, markerMode);
                    }}
                  >
                    {parityType && (
                      <span className={`parity-marker ${parityType}`} aria-hidden="true"></span>
                    )}
                    <input
                      value={cell === 0 ? '' : cell}
                      onChange={(e) => updateCell(r, c, e.target.value)}
                      className="cell input-cell"
                      maxLength={1}
                      inputMode="numeric"
                    />
                  </div>
                );
              })
            )}
          </div>

          <div className="actions">
            <button onClick={solvePuzzle} disabled={!canSolve}>
              {loading ? 'Solving...' : 'Start Solver'}
            </button>
            <button className="secondary" onClick={clearGrid} disabled={loading}>Clear Grid</button>
          </div>

          {error && <div className="error">{error}</div>}
        </section>

        {category !== 'even-odd' && (
          <section className="panel">
            <h2>Variant Constraints JSON</h2>
            <p className="hint">Edit this JSON according to selected category. Coordinates are 1-based: [row, col].</p>
            <textarea
              className="json-box"
              value={variantText}
              onChange={(e) => setVariantText(e.target.value)}
              onBlur={compactVariantJson}
              spellCheck={false}
            />
            {parsedVariant.error ? (
              <div className="error small">{parsedVariant.error}</div>
            ) : (
              <div className="constraint-summary">
                <h3>Readable Summary</h3>
                {variantSummary.length ? (
                  <ul>
                    {variantSummary.map((line, idx) => (
                      <li key={idx}>{line}</li>
                    ))}
                  </ul>
                ) : (
                  <p>No variant constraints set.</p>
                )}
              </div>
            )}
          </section>
        )}
      </main>

      {result && (
        <section className="panel result">
          <h2>Solved Grid</h2>
          <div className="sudoku-grid solved-sudoku-grid">
            {result.solvedGrid.map((row, r) =>
              row.map((cell, c) => {
                const parityType = getParityType(r, c, parsedVariant.data);
                return (
                  <div key={`solved-${r}-${c}`} className={`grid-cell ${cellBorderClasses(r, c)}`}>
                    {parityType && (
                      <span className={`parity-marker ${parityType}`} aria-hidden="true"></span>
                    )}
                    <div
                      className={`cell solved-cell ${grid[r][c] ? 'given' : ''}`}
                    >
                      {cell}
                    </div>
                  </div>
                );
              })
            )}
          </div>
          <p><strong>Unique:</strong> {String(result.unique)}</p>

          <h3>Step-by-step Glassbox Log</h3>
          <div className="log-box">
            {result.logs.map((line, idx) => (
              <div key={idx}>{idx + 1}. {line}</div>
            ))}
          </div>
        </section>
      )}
    </div>
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);
