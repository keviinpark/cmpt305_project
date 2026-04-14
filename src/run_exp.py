import subprocess
import csv
import os

# --- Configuration ---
EXECUTABLE = "./proj"
TRACES = ["srv_0", "compute_fp_1", "compute_int_0"]  # Adjusted to common SFU names
DEPTHS = [1, 2, 3, 4]
REPLICATIONS = [0, 5000000, 10000000, 15000000, 20000000, 25000000]
INST_COUNT = 1000000
OUTPUT_CSV = "two_factor_data.csv"


def run_all():
    headers = [
        "Trace",
        "D",
        "Start_Inst",
        "Cycles",
        "Exec_Time_ms",
        "Int_Pct",
        "FP_Pct",
        "Branch_Pct",
        "Load_Pct",
        "Store_Pct",
    ]

    with open(OUTPUT_CSV, mode="w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(headers)

        for trace in TRACES:
            for d in DEPTHS:
                print(f"\n--- Testing Trace: {trace} | Depth: {d} ---")
                for start in REPLICATIONS:
                    cmd = [EXECUTABLE, trace, str(start), str(INST_COUNT), str(d)]

                    print(f"  Rep {start//1000000}M...", end="", flush=True)

                    try:
                        # Capture output
                        proc = subprocess.run(
                            cmd, capture_output=True, text=True, check=True
                        )
                        row = parse_report(proc.stdout, trace, d, start)
                        writer.writerow(row)
                        print(" Success.")
                    except subprocess.CalledProcessError as e:
                        print(f" Error!\n{e.stderr}")


def parse_report(output, trace, d, start):
    """Parses your specific print_stats() output."""
    cycles, time = 0, 0.0
    p_int, p_fp, p_br, p_ld, p_st = 0.0, 0.0, 0.0, 0.0, 0.0

    for line in output.split("\n"):
        if "Total cycles:" in line:
            cycles = line.split(":")[-1].strip()

        # Match "Execution time: 1.234 ms" or "Execution time: 1.234"
        elif "Execution time:" in line:
            # We strip out 'ms' if you included it in the string
            val = line.split(":")[-1].replace("ms", "").strip()
            time = val

        # Handling histogram lines like "%int : 500000 (50.00%)"
        # We find the value inside the parentheses
        elif "(" in line and "%" in line:
            # Extract value between '(' and '%'
            try:
                pct_value = line.split("(")[1].split("%")[0]
                if "Integer" in line:
                    p_int = pct_value
                elif "Floating Point" in line:
                    p_fp = pct_value
                elif "Branch" in line:
                    p_br = pct_value
                elif "Load" in line:
                    p_ld = pct_value
                elif "Store" in line:
                    p_st = pct_value
            except IndexError:
                pass

    return [trace, d, start, cycles, time, p_int, p_fp, p_br, p_ld, p_st]


if __name__ == "__main__":
    if not os.path.exists(EXECUTABLE):
        print(f"Error: {EXECUTABLE} not found. Did you run 'make'?")
    else:
        run_all()
        print(f"\nSimulation complete. Data saved to {OUTPUT_CSV}")
