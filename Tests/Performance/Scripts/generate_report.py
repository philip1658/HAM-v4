#!/usr/bin/env python3
"""
Generate performance report from benchmark results
"""

import json
import sys
import argparse
from pathlib import Path
from datetime import datetime
import statistics

# Performance thresholds from HAM requirements
THRESHOLDS = {
    'cpu_usage_percent': 5.0,
    'midi_jitter_ms': 0.1,
    'audio_latency_ms': 5.0,
    'memory_mb': 128.0
}

def load_results(filename):
    """Load benchmark results from JSON file"""
    with open(filename, 'r') as f:
        return json.load(f)

def analyze_benchmark(benchmark):
    """Analyze a single benchmark result"""
    analysis = {
        'name': benchmark['name'],
        'real_time_ms': benchmark.get('real_time', 0) / 1_000_000,  # Convert ns to ms
        'cpu_time_ms': benchmark.get('cpu_time', 0) / 1_000_000,
        'iterations': benchmark.get('iterations', 0),
        'violations': []
    }
    
    # Check custom HAM metrics
    if 'cpu_usage_percent' in benchmark:
        analysis['cpu_usage_percent'] = benchmark['cpu_usage_percent']
        if benchmark['cpu_usage_percent'] > THRESHOLDS['cpu_usage_percent']:
            analysis['violations'].append(f"CPU usage {benchmark['cpu_usage_percent']:.2f}% exceeds {THRESHOLDS['cpu_usage_percent']}%")
    
    if 'midi_jitter_ms' in benchmark:
        analysis['midi_jitter_ms'] = benchmark['midi_jitter_ms']
        if benchmark['midi_jitter_ms'] > THRESHOLDS['midi_jitter_ms']:
            analysis['violations'].append(f"MIDI jitter {benchmark['midi_jitter_ms']:.3f}ms exceeds {THRESHOLDS['midi_jitter_ms']}ms")
    
    if 'audio_latency_ms' in benchmark:
        analysis['audio_latency_ms'] = benchmark['audio_latency_ms']
        if benchmark['audio_latency_ms'] > THRESHOLDS['audio_latency_ms']:
            analysis['violations'].append(f"Audio latency {benchmark['audio_latency_ms']:.2f}ms exceeds {THRESHOLDS['audio_latency_ms']}ms")
    
    if 'memory_mb' in benchmark:
        analysis['memory_mb'] = benchmark['memory_mb']
        if benchmark['memory_mb'] > THRESHOLDS['memory_mb']:
            analysis['violations'].append(f"Memory usage {benchmark['memory_mb']:.2f}MB exceeds {THRESHOLDS['memory_mb']}MB")
    
    analysis['status'] = 'PASS' if not analysis['violations'] else 'FAIL'
    
    return analysis

def generate_html_report(results, output_file):
    """Generate HTML report"""
    
    analyses = [analyze_benchmark(b) for b in results.get('benchmarks', [])]
    
    # Calculate summary statistics
    total_benchmarks = len(analyses)
    failed_benchmarks = sum(1 for a in analyses if a['status'] == 'FAIL')
    pass_rate = ((total_benchmarks - failed_benchmarks) / total_benchmarks * 100) if total_benchmarks > 0 else 0
    
    # Group benchmarks by category
    categories = {}
    for analysis in analyses:
        # Extract category from benchmark name (e.g., "BM_AudioProcessor_ProcessBlock/0" -> "AudioProcessor")
        parts = analysis['name'].split('_')
        if len(parts) > 1:
            category = parts[1].split('/')[0]
        else:
            category = 'Other'
        
        if category not in categories:
            categories[category] = []
        categories[category].append(analysis)
    
    html = f"""
<!DOCTYPE html>
<html>
<head>
    <title>HAM Performance Report</title>
    <meta charset="utf-8">
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background: #f5f5f5;
        }}
        .header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            border-radius: 10px;
            margin-bottom: 30px;
        }}
        .summary {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }}
        .summary-card {{
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .summary-card h3 {{
            margin-top: 0;
            color: #666;
            font-size: 14px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }}
        .summary-value {{
            font-size: 32px;
            font-weight: bold;
            color: #333;
        }}
        .pass {{ color: #10b981; }}
        .fail {{ color: #ef4444; }}
        .warning {{ color: #f59e0b; }}
        .category {{
            background: white;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .category h2 {{
            margin-top: 0;
            color: #333;
            border-bottom: 2px solid #e5e7eb;
            padding-bottom: 10px;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
        }}
        th {{
            background: #f9fafb;
            padding: 12px;
            text-align: left;
            font-weight: 600;
            color: #666;
            border-bottom: 2px solid #e5e7eb;
        }}
        td {{
            padding: 12px;
            border-bottom: 1px solid #e5e7eb;
        }}
        tr:hover {{
            background: #f9fafb;
        }}
        .status-badge {{
            display: inline-block;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 12px;
            font-weight: 600;
            text-transform: uppercase;
        }}
        .status-pass {{
            background: #d1fae5;
            color: #065f46;
        }}
        .status-fail {{
            background: #fee2e2;
            color: #991b1b;
        }}
        .violations {{
            color: #ef4444;
            font-size: 12px;
            margin-top: 4px;
        }}
        .footer {{
            text-align: center;
            color: #666;
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid #e5e7eb;
        }}
        .threshold-info {{
            background: #fef3c7;
            border-left: 4px solid #f59e0b;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 4px;
        }}
        .metric-value {{
            font-family: 'Monaco', 'Menlo', monospace;
            font-size: 14px;
        }}
    </style>
</head>
<body>
    <div class="header">
        <h1>🎵 HAM Performance Report</h1>
        <p>Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
    </div>
    
    <div class="summary">
        <div class="summary-card">
            <h3>Total Benchmarks</h3>
            <div class="summary-value">{total_benchmarks}</div>
        </div>
        <div class="summary-card">
            <h3>Pass Rate</h3>
            <div class="summary-value {'pass' if pass_rate == 100 else 'warning' if pass_rate >= 80 else 'fail'}">{pass_rate:.1f}%</div>
        </div>
        <div class="summary-card">
            <h3>Failed</h3>
            <div class="summary-value {'pass' if failed_benchmarks == 0 else 'fail'}">{failed_benchmarks}</div>
        </div>
        <div class="summary-card">
            <h3>Status</h3>
            <div class="summary-value {'pass' if failed_benchmarks == 0 else 'fail'}">
                {'✅ PASS' if failed_benchmarks == 0 else '❌ FAIL'}
            </div>
        </div>
    </div>
    
    <div class="threshold-info">
        <strong>⚠️ Performance Thresholds:</strong>
        CPU Usage &lt; {THRESHOLDS['cpu_usage_percent']}% | 
        MIDI Jitter &lt; {THRESHOLDS['midi_jitter_ms']}ms | 
        Audio Latency &lt; {THRESHOLDS['audio_latency_ms']}ms | 
        Memory &lt; {THRESHOLDS['memory_mb']}MB
    </div>
"""
    
    # Generate tables for each category
    for category, benchmarks in sorted(categories.items()):
        html += f"""
    <div class="category">
        <h2>{category} Benchmarks</h2>
        <table>
            <thead>
                <tr>
                    <th>Benchmark</th>
                    <th>Time (ms)</th>
                    <th>CPU Usage</th>
                    <th>Memory</th>
                    <th>Jitter</th>
                    <th>Status</th>
                </tr>
            </thead>
            <tbody>
"""
        for benchmark in benchmarks:
            status_class = 'pass' if benchmark['status'] == 'PASS' else 'fail'
            html += f"""
                <tr>
                    <td>{benchmark['name'].split('/')[-1] if '/' in benchmark['name'] else benchmark['name']}</td>
                    <td class="metric-value">{benchmark['real_time_ms']:.3f}</td>
                    <td class="metric-value">{benchmark.get('cpu_usage_percent', 'N/A'):.2f}%' if isinstance(benchmark.get('cpu_usage_percent'), (int, float)) else 'N/A'}</td>
                    <td class="metric-value">{benchmark.get('memory_mb', 'N/A'):.2f} MB' if isinstance(benchmark.get('memory_mb'), (int, float)) else 'N/A'}</td>
                    <td class="metric-value">{benchmark.get('midi_jitter_ms', 'N/A'):.4f}' if isinstance(benchmark.get('midi_jitter_ms'), (int, float)) else 'N/A'}</td>
                    <td>
                        <span class="status-badge status-{status_class}">{benchmark['status']}</span>
                        {'<div class="violations">' + '<br>'.join(benchmark['violations']) + '</div>' if benchmark['violations'] else ''}
                    </td>
                </tr>
"""
        html += """
            </tbody>
        </table>
    </div>
"""
    
    html += """
    <div class="footer">
        <p>HAM Performance Benchmark Suite v1.0 | Happy Accident Machine</p>
    </div>
</body>
</html>
"""
    
    with open(output_file, 'w') as f:
        f.write(html)
    
    return failed_benchmarks == 0

def main():
    parser = argparse.ArgumentParser(description='Generate HAM performance report')
    parser.add_argument('input', help='Input JSON file with benchmark results')
    parser.add_argument('--output', '-o', default='performance_report.html', help='Output HTML file')
    parser.add_argument('--fail-on-violations', action='store_true', help='Exit with error if violations found')
    
    args = parser.parse_args()
    
    try:
        results = load_results(args.input)
        success = generate_html_report(results, args.output)
        
        print(f"✅ Report generated: {args.output}")
        
        if args.fail_on_violations and not success:
            print("❌ Performance violations detected!")
            sys.exit(1)
            
    except Exception as e:
        print(f"❌ Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()