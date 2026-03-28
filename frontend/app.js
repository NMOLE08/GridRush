const { useMemo, useState, useEffect, useLayoutEffect, useRef } = React;

function ParticlesBackground({
  colors = ['#efbc9e', '#e9bda6', '#dbc2ad'],
  size = 4.2,
  countDesktop = 84,
  countTablet = 64,
  countMobile = 46,
  zIndex = 0,
  height = '100vh',
}) {
  useLayoutEffect(() => {
    const scriptId = 'particles-js-script';
    const existingScript = document.getElementById(scriptId);

    const getParticleCount = () => {
      const screenWidth = window.innerWidth;
      if (screenWidth > 1024) return countDesktop;
      if (screenWidth > 768) return countTablet;
      return countMobile;
    };

    const initParticles = () => {
      const particlesElement = document.getElementById('js-particles');
      if (!particlesElement || !window.particlesJS) return;

      window.particlesJS('js-particles', {
        particles: {
          number: { value: getParticleCount() },
          color: { value: colors },
          shape: { type: 'circle' },
          opacity: { value: 0.45, random: true },
          size: { value: size, random: true },
          line_linked: { enable: false },
          move: {
            enable: true,
              speed: 2.1,
            direction: 'none',
            random: true,
            straight: false,
            out_mode: 'out',
          },
        },
        interactivity: {
          detect_on: 'canvas',
          events: {
            onhover: { enable: false },
            onclick: { enable: false },
            resize: true,
          },
        },
        retina_detect: true,
      });
    };

    if (window.particlesJS) {
      initParticles();
      return undefined;
    }

    const script = existingScript || document.createElement('script');
    if (!existingScript) {
      script.id = scriptId;
      script.src = 'https://cdn.jsdelivr.net/particles.js/2.0.0/particles.min.js';
      document.body.appendChild(script);
    }

    script.addEventListener('load', initParticles);

    return () => {
      script.removeEventListener('load', initParticles);
    };
  }, [colors, size, countDesktop, countTablet, countMobile]);

  return (
    <div
      id="js-particles"
      style={{
        width: '100%',
        height,
        position: 'fixed',
        top: 0,
        left: 0,
        zIndex,
        pointerEvents: 'none',
      }}
      aria-hidden="true"
    />
  );
}

const CATEGORIES = [
  { id: 'classic', title: 'Classic Sudoku', subtitle: 'Row / Column / Box constraints' },
  { id: 'even-odd', title: 'Even/Odd Sudoku', subtitle: 'Parity markers + classic rules' },
  { id: 'killer', title: 'Killer Sudoku', subtitle: 'Cage sums + no repeats in cage' },
  { id: 'thermo', title: 'Thermo Sudoku', subtitle: 'Strictly increasing along thermometers' },
  { id: 'arrow', title: 'Arrow Sudoku', subtitle: 'Arrow path sums to circle cell' },
  { id: 'kropki', title: 'Kropki Sudoku', subtitle: 'Consecutive and doubling dots' },
  { id: 'hybrid', title: 'Hybrid', subtitle: 'Mix multiple variants together' },
];

const VARIANT_GUIDE = {
  classic: {
    goal: 'Fill the 9x9 standard grid with numbers from 1 to 9.',
    rules: [
      'Every row, every column, and every 3x3 box must contain each number exactly once. No repeats are allowed in these areas.'
    ]
  },
  'even-odd': {
    goal: 'Pay attention to the special shapes drawn inside certain cells.',
    rules: [
      'Squares: If a cell has a square in it, you can only place an even number there (2, 4, 6, or 8).',
      'Circles: If a cell has a circle in it, you can only place an odd number there (1, 3, 5, 7, or 9).'
    ]
  },
  killer: {
    goal: 'Watch out for the dashed outlines grouping cells together, known as killer cages.',
    rules: [
      'The small number shown in the cage indicates the exact sum of all the digits inside that specific cage.',
      'You cannot repeat any digits within a single cage.'
    ]
  },
  thermo: {
    goal: 'You will see thermometer shapes placed across the grid.',
    rules: [
      'Numbers placed on the thermometer must always increase as you move away from the round bulb.'
    ],
    example: 'For example, if the bulb is a 2, the next numbers along the line could be 4, 5, and 8.'
  },
  arrow: {
    goal: 'Look for circles with lines (arrows) extending out from them.',
    rules: [
      'The numbers you place along the arrow path must add up to the exact value inside the circle.',
      'Unlike Killer Sudoku, digits on an arrow are allowed to repeat, as long as repeating them does not break standard Sudoku rules.'
    ]
  },
  kropki: {
    goal: 'Pay close attention to the small black or white dots placed between two neighboring cells.',
    rules: [
      'Black dots: One of the digits in these two adjoining cells must be exactly double the other (for example, 2 and 4, or 3 and 6).',
      'White dots: The digits in these two cells must be consecutive numbers (for example, 4 and 5, or 8 and 9).',
      'Not all possible dots are shown on the board. Two neighboring cells can still be consecutive even without a white dot between them.'
    ]
  },
  hybrid: {
    goal: 'Prepare for the ultimate challenge. A single puzzle can contain a hybrid combination of any of the rules listed above.',
    rules: [
      'Your final solution must satisfy every active variant rule simultaneously.'
    ]
  }
};

const VARIANT_TEMPLATES = {
  classic: {},
  'even-odd': {},
  killer: {},
  thermo: {},
  arrow: {},
  kropki: {},
  hybrid: {}
};

const GRID_CELL_SIZE = 38;
const GRID_GAP = 2;
const GRID_PADDING = 4;
const GRID_PIXEL_SIZE = (GRID_PADDING * 2) + (GRID_CELL_SIZE * 9) + (GRID_GAP * 8);

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

function buildKillerCageVisuals(variantData) {
  const visuals = {};
  if (!variantData || !Array.isArray(variantData.killer_cages)) {
    return visuals;
  }

  for (const cage of variantData.killer_cages) {
    if (!cage || !Array.isArray(cage.cells) || cage.cells.length === 0) continue;

    const normalized = cage.cells
      .filter((cell) => Array.isArray(cell) && cell.length >= 2)
      .map((cell) => ({
        r: Number(cell[0]) - 1,
        c: Number(cell[1]) - 1,
      }))
      .filter((cell) => Number.isInteger(cell.r) && Number.isInteger(cell.c) && cell.r >= 0 && cell.r < 9 && cell.c >= 0 && cell.c < 9);

    if (!normalized.length) continue;

    const cellSet = new Set(normalized.map((cell) => `${cell.r}-${cell.c}`));
    const anchor = normalized.reduce((best, current) => {
      if (!best) return current;
      if (current.r < best.r) return current;
      if (current.r === best.r && current.c < best.c) return current;
      return best;
    }, null);

    for (const cell of normalized) {
      const key = `${cell.r}-${cell.c}`;
      if (!visuals[key]) {
        visuals[key] = {
          top: false,
          right: false,
          bottom: false,
          left: false,
          label: '',
        };
      }

      visuals[key].top = visuals[key].top || !cellSet.has(`${cell.r - 1}-${cell.c}`);
      visuals[key].right = visuals[key].right || !cellSet.has(`${cell.r}-${cell.c + 1}`);
      visuals[key].bottom = visuals[key].bottom || !cellSet.has(`${cell.r + 1}-${cell.c}`);
      visuals[key].left = visuals[key].left || !cellSet.has(`${cell.r}-${cell.c - 1}`);
    }

    const anchorKey = `${anchor.r}-${anchor.c}`;
    if (!visuals[anchorKey]) {
      visuals[anchorKey] = { top: true, right: true, bottom: true, left: true, label: '' };
    }
    visuals[anchorKey].label = String(cage.sum ?? '');
  }

  return visuals;
}

function normalizeThermos(variantData) {
  if (!variantData || !Array.isArray(variantData.thermos)) {
    return [];
  }

  return variantData.thermos
    .map((thermo) => {
      if (!thermo || !Array.isArray(thermo.cells)) return [];
      return thermo.cells
        .filter((cell) => Array.isArray(cell) && cell.length >= 2)
        .map((cell) => ({ r: Number(cell[0]) - 1, c: Number(cell[1]) - 1 }))
        .filter((cell) => Number.isInteger(cell.r) && Number.isInteger(cell.c) && cell.r >= 0 && cell.r < 9 && cell.c >= 0 && cell.c < 9);
    })
    .filter((cells) => cells.length > 0);
}

function normalizeArrows(variantData) {
  if (!variantData || !Array.isArray(variantData.arrows)) {
    return [];
  }

  return variantData.arrows
    .map((arrow) => {
      if (!arrow || !Array.isArray(arrow.circle) || !Array.isArray(arrow.path)) {
        return null;
      }

      const circle = {
        r: Number(arrow.circle[0]) - 1,
        c: Number(arrow.circle[1]) - 1,
      };
      if (!Number.isInteger(circle.r) || !Number.isInteger(circle.c) || circle.r < 0 || circle.r > 8 || circle.c < 0 || circle.c > 8) {
        return null;
      }

      const path = arrow.path
        .filter((cell) => Array.isArray(cell) && cell.length >= 2)
        .map((cell) => ({ r: Number(cell[0]) - 1, c: Number(cell[1]) - 1 }))
        .filter((cell) => Number.isInteger(cell.r) && Number.isInteger(cell.c) && cell.r >= 0 && cell.r < 9 && cell.c >= 0 && cell.c < 9);

      if (!path.length) return null;
      return { circle, path };
    })
    .filter(Boolean);
}

function normalizeKropkiDots(variantData) {
  if (!variantData || !Array.isArray(variantData.kropki)) {
    return [];
  }

  return variantData.kropki
    .map((dot) => {
      if (!dot || !Array.isArray(dot.a) || !Array.isArray(dot.b)) {
        return null;
      }

      const a = { r: Number(dot.a[0]) - 1, c: Number(dot.a[1]) - 1 };
      const b = { r: Number(dot.b[0]) - 1, c: Number(dot.b[1]) - 1 };
      const type = String(dot.type || '').toLowerCase();

      if (!Number.isInteger(a.r) || !Number.isInteger(a.c) || !Number.isInteger(b.r) || !Number.isInteger(b.c)) {
        return null;
      }
      if (a.r < 0 || a.r > 8 || a.c < 0 || a.c > 8 || b.r < 0 || b.r > 8 || b.c < 0 || b.c > 8) {
        return null;
      }
      // Kropki dots are between neighboring orthogonally adjacent cells.
      if (Math.abs(a.r - b.r) + Math.abs(a.c - b.c) !== 1) {
        return null;
      }
      if (type !== 'white' && type !== 'black') {
        return null;
      }

      return { a, b, type };
    })
    .filter(Boolean);
}

function gridCellCenter(cell) {
  return {
    x: GRID_PADDING + (cell.c * (GRID_CELL_SIZE + GRID_GAP)) + (GRID_CELL_SIZE / 2),
    y: GRID_PADDING + (cell.r * (GRID_CELL_SIZE + GRID_GAP)) + (GRID_CELL_SIZE / 2),
  };
}

function GridHoverTrailOverlay({
  overlayKey,
  trailLength = 5,
}) {
  const canvasRef = useRef(null);
  const trailRef = useRef([]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return undefined;

    const gridRoot = canvas.closest('.sudoku-grid');
    if (!gridRoot) return undefined;

    const ctx = canvas.getContext('2d');
    if (!ctx) return undefined;

    const step = GRID_CELL_SIZE + GRID_GAP;
    const width = GRID_PIXEL_SIZE;
    const height = GRID_PIXEL_SIZE;
    canvas.width = width;
    canvas.height = height;

    const pushTrail = (x, y, maxLength = trailLength) => {
      const last = trailRef.current[0];
      if (!last || last.x !== x || last.y !== y) {
        trailRef.current.unshift({ x, y });
        if (trailRef.current.length > maxLength) {
          trailRef.current.pop();
        }
      }
    };

    const handleMove = (event) => {
      const rect = gridRoot.getBoundingClientRect();
      const rawX = event.clientX - rect.left;
      const rawY = event.clientY - rect.top;

      if (rawX < GRID_PADDING || rawY < GRID_PADDING || rawX > width - GRID_PADDING || rawY > height - GRID_PADDING) {
        return;
      }

      const x = Math.max(0, Math.min(8, Math.floor((rawX - GRID_PADDING) / step)));
      const y = Math.max(0, Math.min(8, Math.floor((rawY - GRID_PADDING) / step)));
      pushTrail(x, y);
    };

    const handleLeave = () => {
      trailRef.current = [];
      ctx.clearRect(0, 0, width, height);
    };

    gridRoot.addEventListener('mousemove', handleMove);
    gridRoot.addEventListener('mouseleave', handleLeave);

    let animationFrameId = 0;
    const draw = () => {
      ctx.clearRect(0, 0, width, height);

      trailRef.current.forEach((cell, idx) => {
        const alpha = Math.max(0.08, 0.62 - idx * 0.11);
        const left = GRID_PADDING + (cell.x * step);
        const top = GRID_PADDING + (cell.y * step);

        ctx.fillStyle = `rgba(239, 188, 158, ${alpha})`;
        ctx.shadowColor = `rgba(233, 189, 166, ${Math.min(0.9, alpha + 0.08)})`;
        ctx.shadowBlur = 16;
        ctx.fillRect(left, top, GRID_CELL_SIZE, GRID_CELL_SIZE);
      });

      animationFrameId = window.requestAnimationFrame(draw);
    };

    draw();

    return () => {
      window.cancelAnimationFrame(animationFrameId);
      gridRoot.removeEventListener('mousemove', handleMove);
      gridRoot.removeEventListener('mouseleave', handleLeave);
    };
  }, [overlayKey, trailLength]);

  return <canvas ref={canvasRef} className="grid-hover-effect" aria-hidden="true" />;
}

function ThermoOverlay({ thermos, prefix }) {
  if (!Array.isArray(thermos) || !thermos.length) return null;

  return (
    <svg className="thermo-overlay" viewBox={`0 0 ${GRID_PIXEL_SIZE} ${GRID_PIXEL_SIZE}`} preserveAspectRatio="none" aria-hidden="true">
      {thermos.map((cells, thermoIdx) => {
        const bulbCell = cells[0];
        const bulb = gridCellCenter(bulbCell);

        return (
          <g key={`${prefix}-thermo-${thermoIdx}`}>
            {cells.slice(1).map((cell, segIdx) => {
              const from = gridCellCenter(cells[segIdx]);
              const to = gridCellCenter(cell);
              return (
                <line
                  key={`${prefix}-line-${thermoIdx}-${segIdx}`}
                  x1={from.x}
                  y1={from.y}
                  x2={to.x}
                  y2={to.y}
                  className="thermo-line"
                />
              );
            })}
            <circle cx={bulb.x} cy={bulb.y} r={10} className="thermo-bulb-shape" />
          </g>
        );
      })}
    </svg>
  );
}

function ArrowOverlay({ arrows, prefix }) {
  if (!Array.isArray(arrows) || !arrows.length) return null;

  return (
    <svg className="arrow-overlay" viewBox={`0 0 ${GRID_PIXEL_SIZE} ${GRID_PIXEL_SIZE}`} preserveAspectRatio="none" aria-hidden="true">
      {arrows.map((arrow, idx) => {
        const points = [arrow.circle, ...arrow.path].map((cell) => gridCellCenter(cell));
        const circleCenter = gridCellCenter(arrow.circle);

        let pathPoints = points;
        let headPoints = '';

        if (points.length >= 2) {
          const tip = points[points.length - 1];
          const prev = points[points.length - 2];
          const dx = tip.x - prev.x;
          const dy = tip.y - prev.y;
          const segLen = Math.hypot(dx, dy);

          if (segLen > 0) {
            const ux = dx / segLen;
            const uy = dy / segLen;
            const headLength = 20;
            const headWidth = 18;

            const baseX = tip.x - (ux * headLength);
            const baseY = tip.y - (uy * headLength);
            const leftX = baseX - (uy * headWidth * 0.5);
            const leftY = baseY + (ux * headWidth * 0.5);
            const rightX = baseX + (uy * headWidth * 0.5);
            const rightY = baseY - (ux * headWidth * 0.5);

            pathPoints = [...points.slice(0, -1), { x: baseX, y: baseY }];
            headPoints = `${tip.x},${tip.y} ${leftX},${leftY} ${rightX},${rightY}`;
          }
        }

        const d = pathPoints.map((p, i) => `${i === 0 ? 'M' : 'L'} ${p.x} ${p.y}`).join(' ');

        return (
          <g key={`${prefix}-arrow-${idx}`}>
            <path d={d} className="arrow-line" />
            {headPoints && <polygon points={headPoints} className="arrow-head-shape" />}
            <circle cx={circleCenter.x} cy={circleCenter.y} r={12} className="arrow-circle-shape" />
          </g>
        );
      })}
    </svg>
  );
}

function KropkiOverlay({ dots, prefix }) {
  if (!Array.isArray(dots) || !dots.length) return null;

  return (
    <svg className="kropki-overlay" viewBox={`0 0 ${GRID_PIXEL_SIZE} ${GRID_PIXEL_SIZE}`} preserveAspectRatio="none" aria-hidden="true">
      {dots.map((dot, idx) => {
        const a = gridCellCenter(dot.a);
        const b = gridCellCenter(dot.b);
        const cx = (a.x + b.x) / 2;
        const cy = (a.y + b.y) / 2;
        return (
          <circle
            key={`${prefix}-kropki-${idx}`}
            cx={cx}
            cy={cy}
            r={5.2}
            className={`kropki-dot ${dot.type}`}
          />
        );
      })}
    </svg>
  );
}

function VariantGuide({ category }) {
  const categories = category
    ? CATEGORIES.filter((item) => item.id === category)
    : CATEGORIES;

  return (
    <section className="panel guide-panel">
      <h2>How Each Sudoku Type Works</h2>
      <div className="guide-grid">
        {categories.map((item) => {
          const details = VARIANT_GUIDE[item.id];
          if (!details) return null;

          return (
            <article key={item.id} className="guide-card">
              <h3>{item.title}</h3>
              <p><strong>The Goal:</strong> {details.goal}</p>
              <ul>
                {details.rules.map((rule, idx) => (
                  <li key={idx}>{rule}</li>
                ))}
              </ul>
              {details.example && <p className="guide-example"><strong>Example:</strong> {details.example}</p>}
            </article>
          );
        })}
      </div>
    </section>
  );
}

function App() {
  const [category, setCategory] = useState(null);
  const [grid, setGrid] = useState(emptyGrid());
  const [variantText, setVariantText] = useState('{}');
  const [markerMode, setMarkerMode] = useState('');
  const [killerEditMode, setKillerEditMode] = useState(false);
  const [killerSelection, setKillerSelection] = useState([]);
  const [killerSumInput, setKillerSumInput] = useState('');
  const [thermoEditMode, setThermoEditMode] = useState(false);
  const [thermoPathSelection, setThermoPathSelection] = useState([]);
  const [arrowEditMode, setArrowEditMode] = useState(false);
  const [arrowCircleSelection, setArrowCircleSelection] = useState('');
  const [arrowPathSelection, setArrowPathSelection] = useState([]);
  const [kropkiEditMode, setKropkiEditMode] = useState(false);
  const [kropkiPairSelection, setKropkiPairSelection] = useState([]);
  const [kropkiDotType, setKropkiDotType] = useState('white');
  const [loading, setLoading] = useState(false);
  const [uniquenessLoading, setUniquenessLoading] = useState(false);
  const [classicAnalysisLoading, setClassicAnalysisLoading] = useState(false);
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
    setKillerEditMode(false);
    setKillerSelection([]);
    setKillerSumInput('');
    setThermoEditMode(false);
    setThermoPathSelection([]);
    setArrowEditMode(false);
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setKropkiEditMode(false);
    setKropkiPairSelection([]);
    setKropkiDotType('white');
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
  const supportsKillerCages = useMemo(
    () => category === 'killer' || category === 'hybrid',
    [category]
  );
  const supportsThermoShapes = useMemo(
    () => category === 'thermo' || category === 'hybrid',
    [category]
  );
  const supportsKropkiDots = useMemo(
    () => category === 'kropki' || category === 'hybrid',
    [category]
  );
  const supportsArrowShapes = useMemo(
    () => category === 'arrow' || category === 'hybrid',
    [category]
  );
  const killerCageVisuals = useMemo(
    () => buildKillerCageVisuals(parsedVariant.data || {}),
    [parsedVariant]
  );
  const thermoPaths = useMemo(
    () => normalizeThermos(parsedVariant.data || {}),
    [parsedVariant]
  );
  const arrowPaths = useMemo(
    () => normalizeArrows(parsedVariant.data || {}),
    [parsedVariant]
  );
  const kropkiDots = useMemo(
    () => normalizeKropkiDots(parsedVariant.data || {}),
    [parsedVariant]
  );

  const variantJsonPanel = (
    <section className={`panel ${category === 'hybrid' ? 'hybrid-variant-panel' : ''}`}>
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

  function handleInputArrowNavigation(r, c, event) {
    let nextRow = r;
    let nextCol = c;

    if (event.key === 'ArrowUp') nextRow = Math.max(0, r - 1);
    else if (event.key === 'ArrowDown') nextRow = Math.min(8, r + 1);
    else if (event.key === 'ArrowLeft') nextCol = Math.max(0, c - 1);
    else if (event.key === 'ArrowRight') nextCol = Math.min(8, c + 1);
    else return;

    if (nextRow === r && nextCol === c) return;

    event.preventDefault();
    const gridRoot = event.currentTarget.closest('.sudoku-grid');
    if (!gridRoot) return;

    const selector = `input.input-cell[data-row="${nextRow}"][data-col="${nextCol}"]`;
    const target = gridRoot.querySelector(selector);
    if (target) {
      target.focus();
      target.select();
    }
  }

  function clearGrid() {
    setGrid(emptyGrid());
    setResult(null);
    setError('');
  }

  function clearParityMarkers() {
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    const next = {
      ...parsedVariant.data,
      even_cells: [],
      odd_cells: [],
    };

    setVariantText(JSON.stringify(next));
    setMarkerMode('');
    setError('');
  }

  function clearThermoPaths() {
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    const next = {
      ...parsedVariant.data,
      thermos: [],
    };

    setVariantText(JSON.stringify(next));
    setThermoPathSelection([]);
    setThermoEditMode(false);
    setError('');
  }

  function clearArrowPaths() {
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    const next = {
      ...parsedVariant.data,
      arrows: [],
    };

    setVariantText(JSON.stringify(next));
    setArrowEditMode(false);
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setError('');
  }

  function clearKropkiDots() {
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    const next = {
      ...parsedVariant.data,
      kropki: [],
    };

    setVariantText(JSON.stringify(next));
    setKropkiEditMode(false);
    setKropkiPairSelection([]);
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
          checkUniqueness: false,
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

  async function analyzeClassicSolutions() {
    if (category !== 'classic') return;

    setClassicAnalysisLoading(true);
    setError('');

    try {
      const res = await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          category,
          givensGrid: grid,
          variantData: {},
          checkUniqueness: true,
          checkAllSolutions: true,
          maxSolutionCount: 5000,
          includeLogs: false,
        }),
      });

      const data = await res.json();
      if (!res.ok || !data.ok) {
        throw new Error(data.details || data.error || 'Failed to analyze classic solutions.');
      }

      setResult(data);
    } catch (e) {
      setError(e.message || 'Unknown error while analyzing classic solutions.');
    } finally {
      setClassicAnalysisLoading(false);
    }
  }

  async function analyzeUniqueness() {
    if (!category) return;

    let variantData = {};
    try {
      variantData = variantText.trim() ? JSON.parse(variantText) : {};
    } catch {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    setUniquenessLoading(true);
    setError('');

    try {
      const res = await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          category,
          givensGrid: grid,
          variantData,
          checkUniqueness: true,
          includeLogs: false,
        }),
      });

      const data = await res.json();
      if (!res.ok || !data.ok) {
        throw new Error(data.details || data.error || 'Failed to check uniqueness.');
      }

      setResult(data);
    } catch (e) {
      setError(e.message || 'Unknown error while checking uniqueness.');
    } finally {
      setUniquenessLoading(false);
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
    setKillerEditMode(false);
    setKillerSelection([]);
    setThermoEditMode(false);
    setThermoPathSelection([]);
    setArrowEditMode(false);
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setKropkiEditMode(false);
    setKropkiPairSelection([]);
  }

  function toggleKillerEditMode() {
    setKillerEditMode((current) => !current);
    setMarkerMode('');
    setKillerSelection([]);
    setThermoEditMode(false);
    setThermoPathSelection([]);
    setArrowEditMode(false);
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setKropkiEditMode(false);
    setKropkiPairSelection([]);
  }

  function toggleThermoEditMode() {
    setThermoEditMode((current) => !current);
    setMarkerMode('');
    setKillerEditMode(false);
    setKillerSelection([]);
    setThermoPathSelection([]);
    setArrowEditMode(false);
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setKropkiEditMode(false);
    setKropkiPairSelection([]);
  }

  function toggleArrowEditMode() {
    setArrowEditMode((current) => !current);
    setMarkerMode('');
    setKillerEditMode(false);
    setKillerSelection([]);
    setThermoEditMode(false);
    setThermoPathSelection([]);
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setKropkiEditMode(false);
    setKropkiPairSelection([]);
  }

  function toggleKropkiEditMode() {
    setKropkiEditMode((current) => !current);
    setMarkerMode('');
    setKillerEditMode(false);
    setKillerSelection([]);
    setThermoEditMode(false);
    setThermoPathSelection([]);
    setArrowEditMode(false);
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setKropkiPairSelection([]);
  }

  function toggleKillerCellSelection(r, c) {
    const key = `${r}-${c}`;
    setKillerSelection((current) => {
      if (current.includes(key)) {
        return current.filter((item) => item !== key);
      }
      return [...current, key];
    });
  }

  function applyKillerCageSelection() {
    if (!supportsKillerCages || !killerEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    if (!killerSelection.length) {
      setError('Select at least one cell for the killer cage.');
      return;
    }

    const sum = Number(killerSumInput);
    if (!Number.isInteger(sum) || sum <= 0) {
      setError('Enter a valid cage sum (positive integer).');
      return;
    }

    const sortedKeys = [...killerSelection].sort((a, b) => {
      const [ar, ac] = a.split('-').map(Number);
      const [br, bc] = b.split('-').map(Number);
      if (ar !== br) return ar - br;
      return ac - bc;
    });

    const cells = sortedKeys.map((key) => {
      const [r, c] = key.split('-').map(Number);
      return [r + 1, c + 1];
    });

    const serializeCells = (arr) => arr
      .filter((cell) => Array.isArray(cell) && cell.length >= 2)
      .map((cell) => `${Number(cell[0])}-${Number(cell[1])}`)
      .sort()
      .join('|');

    const next = { ...parsedVariant.data };
    const cages = Array.isArray(next.killer_cages) ? [...next.killer_cages] : [];
    const targetKey = serializeCells(cells);
    const existingIdx = cages.findIndex((cage) => serializeCells(cage?.cells || []) === targetKey);

    const nextCage = { sum, cells };
    if (existingIdx >= 0) {
      cages[existingIdx] = nextCage;
    } else {
      cages.push(nextCage);
    }

    next.killer_cages = cages;
    setVariantText(JSON.stringify(next));
    setError('');
    setKillerSelection([]);
  }

  function removeKillerCageSelection() {
    if (!supportsKillerCages || !killerEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    if (!killerSelection.length) {
      setError('Select cage cells first, then click Remove Cage.');
      return;
    }

    const sortedKeys = [...killerSelection].sort((a, b) => {
      const [ar, ac] = a.split('-').map(Number);
      const [br, bc] = b.split('-').map(Number);
      if (ar !== br) return ar - br;
      return ac - bc;
    });

    const selectedCells = sortedKeys.map((key) => {
      const [r, c] = key.split('-').map(Number);
      return [r + 1, c + 1];
    });

    const serializeCells = (arr) => arr
      .filter((cell) => Array.isArray(cell) && cell.length >= 2)
      .map((cell) => `${Number(cell[0])}-${Number(cell[1])}`)
      .sort()
      .join('|');

    const next = { ...parsedVariant.data };
    const cages = Array.isArray(next.killer_cages) ? [...next.killer_cages] : [];
    const targetKey = serializeCells(selectedCells);

    const filtered = cages.filter((cage) => serializeCells(cage?.cells || []) !== targetKey);
    if (filtered.length === cages.length) {
      setError('No cage found for the selected cells.');
      return;
    }

    next.killer_cages = filtered;
    setVariantText(JSON.stringify(next));
    setError('');
    setKillerSelection([]);
  }

  function addThermoPathCell(r, c) {
    const key = `${r}-${c}`;
    setThermoPathSelection((current) => {
      if (!current.length) {
        return [key];
      }

      if (current[current.length - 1] === key) {
        return current.slice(0, -1);
      }

      if (current.includes(key)) {
        setError('Thermometer path cannot revisit a cell.');
        return current;
      }

      const [lr, lc] = current[current.length - 1].split('-').map(Number);
      const dr = Math.abs(r - lr);
      const dc = Math.abs(c - lc);
      if (dr === 0 && dc === 0) {
        return current;
      }
      if (dr > 1 || dc > 1) {
        setError('Thermometer cells must be adjacent (orthogonal or diagonal) in sequence.');
        return current;
      }

      setError('');
      return [...current, key];
    });
  }

  function applyThermoPathSelection() {
    if (!supportsThermoShapes || !thermoEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    if (thermoPathSelection.length < 2) {
      setError('Select at least 2 connected cells for a thermometer.');
      return;
    }

    const cells = thermoPathSelection.map((key) => {
      const [r, c] = key.split('-').map(Number);
      return [r + 1, c + 1];
    });

    const serializePath = (pathCells) => pathCells
      .filter((cell) => Array.isArray(cell) && cell.length >= 2)
      .map((cell) => `${Number(cell[0])}-${Number(cell[1])}`)
      .join('>');

    const next = { ...parsedVariant.data };
    const thermos = Array.isArray(next.thermos) ? [...next.thermos] : [];
    const targetKey = serializePath(cells);
    const existingIdx = thermos.findIndex((t) => serializePath(t?.cells || []) === targetKey);

    const nextThermo = { cells };
    if (existingIdx >= 0) {
      thermos[existingIdx] = nextThermo;
    } else {
      thermos.push(nextThermo);
    }

    next.thermos = thermos;
    setVariantText(JSON.stringify(next));
    setThermoPathSelection([]);
    setError('');
  }

  function removeThermoPathSelection() {
    if (!supportsThermoShapes || !thermoEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }

    if (thermoPathSelection.length < 2) {
      setError('Select the thermometer path cells to remove.');
      return;
    }

    const selectedCells = thermoPathSelection.map((key) => {
      const [r, c] = key.split('-').map(Number);
      return [r + 1, c + 1];
    });

    const serializeSet = (pathCells) => pathCells
      .filter((cell) => Array.isArray(cell) && cell.length >= 2)
      .map((cell) => `${Number(cell[0])}-${Number(cell[1])}`)
      .sort()
      .join('|');

    const next = { ...parsedVariant.data };
    const thermos = Array.isArray(next.thermos) ? [...next.thermos] : [];
    const targetSet = serializeSet(selectedCells);
    const filtered = thermos.filter((t) => serializeSet(t?.cells || []) !== targetSet);

    if (filtered.length === thermos.length) {
      setError('No thermometer found for the selected cells.');
      return;
    }

    next.thermos = filtered;
    setVariantText(JSON.stringify(next));
    setThermoPathSelection([]);
    setError('');
  }

  function addArrowCellSelection(r, c) {
    const key = `${r}-${c}`;

    if (!arrowCircleSelection) {
      setArrowCircleSelection(key);
      setArrowPathSelection([]);
      setError('');
      return;
    }

    if (arrowCircleSelection === key) {
      setArrowCircleSelection('');
      setArrowPathSelection([]);
      setError('');
      return;
    }

    setArrowPathSelection((current) => {
      if (!current.length) {
        const [cr, cc] = arrowCircleSelection.split('-').map(Number);
        const dr = Math.abs(r - cr);
        const dc = Math.abs(c - cc);
        if (dr > 1 || dc > 1 || (dr === 0 && dc === 0)) {
          setError('First arrow path cell must be adjacent to the circle.');
          return current;
        }
        setError('');
        return [key];
      }

      if (current[current.length - 1] === key) {
        return current.slice(0, -1);
      }

      if (current.includes(key)) {
        setError('Arrow path cannot revisit a cell.');
        return current;
      }

      const [lr, lc] = current[current.length - 1].split('-').map(Number);
      const dr = Math.abs(r - lr);
      const dc = Math.abs(c - lc);
      if (dr > 1 || dc > 1 || (dr === 0 && dc === 0)) {
        setError('Arrow path cells must be adjacent in sequence.');
        return current;
      }

      setError('');
      return [...current, key];
    });
  }

  function applyArrowSelection() {
    if (!supportsArrowShapes || !arrowEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }
    if (!arrowCircleSelection || !arrowPathSelection.length) {
      setError('Select one circle cell and at least one arrow path cell.');
      return;
    }

    const circle = arrowCircleSelection.split('-').map((v) => Number(v) + 1);
    const path = arrowPathSelection.map((key) => {
      const [r, c] = key.split('-').map(Number);
      return [r + 1, c + 1];
    });

    const serializeArrow = (a) => {
      if (!a || !Array.isArray(a.circle) || !Array.isArray(a.path)) return '';
      const circleKey = `${Number(a.circle[0])}-${Number(a.circle[1])}`;
      const pathKey = a.path.map((cell) => `${Number(cell[0])}-${Number(cell[1])}`).join('>');
      return `${circleKey}|${pathKey}`;
    };

    const next = { ...parsedVariant.data };
    const arrows = Array.isArray(next.arrows) ? [...next.arrows] : [];
    const target = { circle, path };
    const targetKey = serializeArrow(target);
    const existingIdx = arrows.findIndex((a) => serializeArrow(a) === targetKey);

    if (existingIdx >= 0) {
      arrows[existingIdx] = target;
    } else {
      arrows.push(target);
    }

    next.arrows = arrows;
    setVariantText(JSON.stringify(next));
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setError('');
  }

  function removeArrowSelection() {
    if (!supportsArrowShapes || !arrowEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }
    if (!arrowCircleSelection || !arrowPathSelection.length) {
      setError('Select one circle and its path cells to remove arrow.');
      return;
    }

    const circle = arrowCircleSelection.split('-').map((v) => Number(v) + 1);
    const path = arrowPathSelection.map((key) => {
      const [r, c] = key.split('-').map(Number);
      return [r + 1, c + 1];
    });

    const serializeArrow = (a) => {
      if (!a || !Array.isArray(a.circle) || !Array.isArray(a.path)) return '';
      const circleKey = `${Number(a.circle[0])}-${Number(a.circle[1])}`;
      const pathKey = a.path.map((cell) => `${Number(cell[0])}-${Number(cell[1])}`).join('>');
      return `${circleKey}|${pathKey}`;
    };

    const next = { ...parsedVariant.data };
    const arrows = Array.isArray(next.arrows) ? [...next.arrows] : [];
    const targetKey = serializeArrow({ circle, path });
    const filtered = arrows.filter((a) => serializeArrow(a) !== targetKey);

    if (filtered.length === arrows.length) {
      setError('No arrow found for current circle/path selection.');
      return;
    }

    next.arrows = filtered;
    setVariantText(JSON.stringify(next));
    setArrowCircleSelection('');
    setArrowPathSelection([]);
    setError('');
  }

  function addKropkiCellSelection(r, c) {
    const key = `${r}-${c}`;
    setKropkiPairSelection((current) => {
      if (!current.length) {
        setError('');
        return [key];
      }

      if (current.length === 1) {
        const [fr, fc] = current[0].split('-').map(Number);
        const dist = Math.abs(fr - r) + Math.abs(fc - c);

        if (current[0] === key) {
          return [];
        }
        if (dist !== 1) {
          setError('Kropki dot must be between orthogonally adjacent cells.');
          return current;
        }
        setError('');
        return [current[0], key];
      }

      if (current.includes(key)) {
        return current.filter((v) => v !== key);
      }

      return [key];
    });
  }

  function applyKropkiDotSelection() {
    if (!supportsKropkiDots || !kropkiEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }
    if (kropkiPairSelection.length !== 2) {
      setError('Select exactly two orthogonally adjacent cells for a Kropki dot.');
      return;
    }

    const cells = kropkiPairSelection
      .map((key) => key.split('-').map((n) => Number(n) + 1))
      .sort((a, b) => {
        if (a[0] !== b[0]) return a[0] - b[0];
        return a[1] - b[1];
      });

    const next = { ...parsedVariant.data };
    const dots = Array.isArray(next.kropki) ? [...next.kropki] : [];
    const pairKey = `${cells[0][0]}-${cells[0][1]}|${cells[1][0]}-${cells[1][1]}`;
    const dotPairKey = (dot) => {
      const pair = [dot.a, dot.b]
        .map((cell) => [Number(cell?.[0]), Number(cell?.[1])])
        .sort((x, y) => (x[0] !== y[0] ? x[0] - y[0] : x[1] - y[1]));
      return `${pair[0][0]}-${pair[0][1]}|${pair[1][0]}-${pair[1][1]}`;
    };

    const idx = dots.findIndex((dot) => dotPairKey(dot) === pairKey);
    const nextDot = { a: cells[0], b: cells[1], type: kropkiDotType };
    if (idx >= 0) {
      dots[idx] = nextDot;
    } else {
      dots.push(nextDot);
    }

    next.kropki = dots;
    setVariantText(JSON.stringify(next));
    setKropkiPairSelection([]);
    setError('');
  }

  function removeKropkiDotSelection() {
    if (!supportsKropkiDots || !kropkiEditMode) return;
    if (!parsedVariant.data) {
      setError('Variant JSON is invalid. Fix it and try again.');
      return;
    }
    if (kropkiPairSelection.length !== 2) {
      setError('Select exactly two cells of the Kropki pair to remove.');
      return;
    }

    const cells = kropkiPairSelection
      .map((key) => key.split('-').map((n) => Number(n) + 1))
      .sort((a, b) => {
        if (a[0] !== b[0]) return a[0] - b[0];
        return a[1] - b[1];
      });

    const next = { ...parsedVariant.data };
    const dots = Array.isArray(next.kropki) ? [...next.kropki] : [];
    const pairKey = `${cells[0][0]}-${cells[0][1]}|${cells[1][0]}-${cells[1][1]}`;
    const dotPairKey = (dot) => {
      const pair = [dot.a, dot.b]
        .map((cell) => [Number(cell?.[0]), Number(cell?.[1])])
        .sort((x, y) => (x[0] !== y[0] ? x[0] - y[0] : x[1] - y[1]));
      return `${pair[0][0]}-${pair[0][1]}|${pair[1][0]}-${pair[1][1]}`;
    };

    const filtered = dots.filter((dot) => dotPairKey(dot) !== pairKey);
    if (filtered.length === dots.length) {
      setError('No Kropki dot found for selected cells.');
      return;
    }

    next.kropki = filtered;
    setVariantText(JSON.stringify(next));
    setKropkiPairSelection([]);
    setError('');
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

      <VariantGuide category={category} />

      <main className={`layout ${category === 'hybrid' ? 'hybrid-main-layout' : ''}`}>
        <section className={`panel ${category === 'hybrid' ? 'hybrid-input-panel' : ''}`}>
          <h2>Input Grid</h2>
          <div className="hybrid-tools-column">
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
            {supportsKillerCages && (
              <div className="killer-tools">
                <p className="hint">Killer Cage Tool: select cells for a cage, set sum, then add/update.</p>
                <div className="killer-tools-row">
                  <button
                    className={`secondary killer-toggle ${killerEditMode ? 'active' : ''}`}
                    onClick={toggleKillerEditMode}
                    type="button"
                  >
                    {killerEditMode ? 'Stop Cage Selection' : 'Select Cage Cells'}
                  </button>
                  <input
                    className="killer-sum-input"
                    type="number"
                    min="1"
                    step="1"
                    value={killerSumInput}
                    onChange={(e) => setKillerSumInput(e.target.value)}
                    placeholder="Cage sum"
                  />
                  <button className="secondary" type="button" onClick={applyKillerCageSelection}>
                    Add/Update Cage
                  </button>
                  <button className="secondary" type="button" onClick={removeKillerCageSelection}>
                    Remove Cage
                  </button>
                  <button className="secondary" type="button" onClick={() => setKillerSelection([])}>
                    Clear Selection
                  </button>
                </div>
              </div>
            )}
            {supportsThermoShapes && (
              <div className="thermo-tools">
                <p className="hint">Thermo Tool: select connected cells from bulb to tip, then add/update.</p>
                <div className="thermo-tools-row">
                  <button
                    className={`secondary thermo-toggle ${thermoEditMode ? 'active' : ''}`}
                    onClick={toggleThermoEditMode}
                    type="button"
                  >
                    {thermoEditMode ? 'Stop Thermo Selection' : 'Select Thermo Path'}
                  </button>
                  <button className="secondary" type="button" onClick={applyThermoPathSelection}>
                    Add/Update Thermo
                  </button>
                  <button className="secondary" type="button" onClick={removeThermoPathSelection}>
                    Remove Thermo
                  </button>
                  <button className="secondary" type="button" onClick={() => setThermoPathSelection([])}>
                    Clear Path
                  </button>
                </div>
              </div>
            )}
            {supportsArrowShapes && (
              <div className="arrow-tools">
                <p className="hint">Arrow Tool: select circle cell first, then select connected path cells.</p>
                <div className="arrow-tools-row">
                  <button
                    className={`secondary arrow-toggle ${arrowEditMode ? 'active' : ''}`}
                    onClick={toggleArrowEditMode}
                    type="button"
                  >
                    {arrowEditMode ? 'Stop Arrow Selection' : 'Select Arrow'}
                  </button>
                  <button className="secondary" type="button" onClick={applyArrowSelection}>
                    Add/Update Arrow
                  </button>
                  <button className="secondary" type="button" onClick={removeArrowSelection}>
                    Remove Arrow
                  </button>
                  <button className="secondary" type="button" onClick={() => { setArrowCircleSelection(''); setArrowPathSelection([]); }}>
                    Clear Arrow Selection
                  </button>
                </div>
              </div>
            )}
            {supportsKropkiDots && (
              <div className="kropki-tools">
                <p className="hint">Kropki Tool: select two adjacent cells, pick dot type, then add/update or remove.</p>
                <div className="kropki-tools-row">
                  <button
                    className={`secondary kropki-toggle ${kropkiEditMode ? 'active' : ''}`}
                    onClick={toggleKropkiEditMode}
                    type="button"
                  >
                    {kropkiEditMode ? 'Stop Kropki Selection' : 'Select Kropki Pair'}
                  </button>
                  <select
                    className="kropki-type-select"
                    value={kropkiDotType}
                    onChange={(e) => setKropkiDotType(e.target.value)}
                  >
                    <option value="white">White Dot (consecutive)</option>
                    <option value="black">Black Dot (double)</option>
                  </select>
                  <button className="secondary" type="button" onClick={applyKropkiDotSelection}>
                    Add/Update Dot
                  </button>
                  <button className="secondary" type="button" onClick={removeKropkiDotSelection}>
                    Remove Dot
                  </button>
                  <button className="secondary" type="button" onClick={() => setKropkiPairSelection([])}>
                    Clear Pair
                  </button>
                </div>
              </div>
            )}
          </div>

          <div className="hybrid-grid-column">
            <div className="sudoku-grid">
              <GridHoverTrailOverlay overlayKey={`input-${category}`} />
              {supportsThermoShapes && <ThermoOverlay thermos={thermoPaths} prefix="input" />}
              {supportsArrowShapes && <ArrowOverlay arrows={arrowPaths} prefix="input" />}
              {supportsKropkiDots && <KropkiOverlay dots={kropkiDots} prefix="input" />}
              {grid.map((row, r) =>
                row.map((cell, c) => {
                  const parityType = getParityType(r, c, parsedVariant.data);
                  const killerVisual = supportsKillerCages ? killerCageVisuals[`${r}-${c}`] : null;
                  const isKillerSelected = killerSelection.includes(`${r}-${c}`);
                  const isThermoSelected = thermoPathSelection.includes(`${r}-${c}`);
                  const thermoStep = thermoPathSelection.indexOf(`${r}-${c}`);
                  const arrowKey = `${r}-${c}`;
                  const isArrowCircleSelected = arrowCircleSelection === arrowKey;
                  const isArrowPathSelected = arrowPathSelection.includes(arrowKey);
                  const isKropkiSelected = kropkiPairSelection.includes(arrowKey);
                  const killerClass = killerVisual
                    ? `killer-cage-outline ${killerVisual.top ? 'top' : ''} ${killerVisual.right ? 'right' : ''} ${killerVisual.bottom ? 'bottom' : ''} ${killerVisual.left ? 'left' : ''}`.trim()
                    : '';
                  return (
                    <div
                      key={`${r}-${c}`}
                      className={`grid-cell ${cellBorderClasses(r, c)} ${markerMode ? 'marker-mode' : ''} ${killerEditMode ? 'killer-mode' : ''} ${isKillerSelected ? 'killer-selected' : ''} ${thermoEditMode ? 'thermo-mode' : ''} ${isThermoSelected ? 'thermo-selected' : ''} ${arrowEditMode ? 'arrow-mode' : ''} ${isArrowCircleSelected ? 'arrow-circle-selected' : ''} ${isArrowPathSelected ? 'arrow-path-selected' : ''} ${kropkiEditMode ? 'kropki-mode' : ''} ${isKropkiSelected ? 'kropki-selected' : ''}`}
                      onMouseDown={(e) => {
                        if (kropkiEditMode && supportsKropkiDots) {
                          e.preventDefault();
                          addKropkiCellSelection(r, c);
                          return;
                        }
                        if (arrowEditMode && supportsArrowShapes) {
                          e.preventDefault();
                          addArrowCellSelection(r, c);
                          return;
                        }
                        if (thermoEditMode && supportsThermoShapes) {
                          e.preventDefault();
                          addThermoPathCell(r, c);
                          return;
                        }
                        if (killerEditMode && supportsKillerCages) {
                          e.preventDefault();
                          toggleKillerCellSelection(r, c);
                          return;
                        }
                        if (!markerMode || !supportsParityMarkers) return;
                        e.preventDefault();
                        updateParityMarker(r, c, markerMode);
                      }}
                    >
                      {killerVisual && (
                        <span className={killerClass} aria-hidden="true"></span>
                      )}
                      {killerVisual?.label && (
                        <span className="killer-cage-sum" aria-hidden="true">{killerVisual.label}</span>
                      )}
                      {thermoStep >= 0 && (
                        <span className="thermo-step-index" aria-hidden="true">{thermoStep + 1}</span>
                      )}
                      {parityType && (
                        <span className={`parity-marker ${parityType}`} aria-hidden="true"></span>
                      )}
                      <input
                        value={cell === 0 ? '' : cell}
                        onChange={(e) => updateCell(r, c, e.target.value)}
                        onKeyDown={(e) => handleInputArrowNavigation(r, c, e)}
                        className="cell input-cell"
                        data-row={r}
                        data-col={c}
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
              {category === 'even-odd' && (
                <button className="secondary" onClick={clearParityMarkers} disabled={loading}>
                  Clear Squares/Circles
                </button>
              )}
              {category === 'thermo' && (
                <button className="secondary" onClick={clearThermoPaths} disabled={loading}>
                  Clear Thermo Paths
                </button>
              )}
              {category === 'arrow' && (
                <button className="secondary" onClick={clearArrowPaths} disabled={loading}>
                  Clear Arrows
                </button>
              )}
              {category === 'kropki' && (
                <button className="secondary" onClick={clearKropkiDots} disabled={loading}>
                  Clear Kropki Dots
                </button>
              )}
            </div>

            {error && <div className="error">{error}</div>}
          </div>
        </section>

        {category !== 'hybrid' && variantJsonPanel}
      </main>

      {category === 'hybrid' && variantJsonPanel}

      {result && (
        <section className="panel result">
          <h2>{result.solved ? 'Solved Grid' : 'Solver Result'}</h2>
          {Array.isArray(result.solvedGrid) ? (
            <div className="sudoku-grid solved-sudoku-grid">
              <GridHoverTrailOverlay overlayKey={`solved-${category}`} />
              {supportsThermoShapes && <ThermoOverlay thermos={thermoPaths} prefix="solved" />}
              {supportsArrowShapes && <ArrowOverlay arrows={arrowPaths} prefix="solved" />}
              {supportsKropkiDots && <KropkiOverlay dots={kropkiDots} prefix="solved" />}
              {result.solvedGrid.map((row, r) =>
                row.map((cell, c) => {
                  const parityType = getParityType(r, c, parsedVariant.data);
                  const killerVisual = supportsKillerCages ? killerCageVisuals[`${r}-${c}`] : null;
                  const killerClass = killerVisual
                    ? `killer-cage-outline ${killerVisual.top ? 'top' : ''} ${killerVisual.right ? 'right' : ''} ${killerVisual.bottom ? 'bottom' : ''} ${killerVisual.left ? 'left' : ''}`.trim()
                    : '';
                  return (
                    <div key={`solved-${r}-${c}`} className={`grid-cell ${cellBorderClasses(r, c)}`}>
                      {killerVisual && (
                        <span className={killerClass} aria-hidden="true"></span>
                      )}
                      {killerVisual?.label && (
                        <span className="killer-cage-sum" aria-hidden="true">{killerVisual.label}</span>
                      )}
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
          ) : (
            <p>{result.message || 'Solver could not find a valid solved grid for the provided constraints.'}</p>
          )}
          <p>
            <strong>Unique:</strong>{' '}
            {result.uniquenessChecked ? String(result.unique) : 'Not checked'}
          </p>
          {typeof result.solutionCount === 'number' && (
            <p>
              <strong>Solution Count:</strong>{' '}
              {result.solutionCount}
              {result.solutionCountComplete === false ? ' (cap reached)' : ''}
            </p>
          )}
          {result.solved && (
            <button
              className="secondary"
              onClick={analyzeUniqueness}
              disabled={loading || uniquenessLoading || classicAnalysisLoading}
            >
              {uniquenessLoading ? 'Checking Uniqueness...' : 'Check Uniqueness'}
            </button>
          )}
          {category === 'classic' && result.solved && (
            <button
              className="secondary"
              onClick={analyzeClassicSolutions}
              disabled={loading || uniquenessLoading || classicAnalysisLoading}
            >
              {classicAnalysisLoading ? 'Checking All Classic Solutions...' : 'Check All Possible Classic Solutions'}
            </button>
          )}

          {!result.logsSuppressed && (
            <>
              <h3>Step-by-step Glassbox Log</h3>
              <div className="log-box">
                {result.logs.map((line, idx) => (
                  <div key={idx}>{idx + 1}. {line}</div>
                ))}
              </div>
            </>
          )}
        </section>
      )}
    </div>
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(
  <>
    <ParticlesBackground />
    <App />
  </>
);
