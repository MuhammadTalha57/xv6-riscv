#!/usr/bin/env python3

#
# MLFQ Test Script - Runs MLFQ tests and generates a report
#

import argparse, os, subprocess, sys, time
from datetime import datetime
from subprocess import run, Popen, PIPE


class TestResult:
    def __init__(self, name, status, output="", error=""):
        self.name = name
        self.status = status  # "PASS", "FAIL", "SKIP", "ERROR"
        self.output = output
        self.error = error
        self.duration = 0


class QEMUTest:
    def __init__(self, reset=False):
        if reset:
            self.build_xv6()
            self.reset_fs()
        q = ["make", "qemu"]
        self.proc = Popen(q, stdin=PIPE, stdout=PIPE, stderr=subprocess.STDOUT)
        self.output = ""
        self.outbytes = bytearray()
        time.sleep(2)  # Give QEMU time to boot

    def reset_fs(self):
        try:
            run(["rm", "-f", "fs.img"], check=False)
            run(["make", "fs.img"], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Warning: fs.img reset failed: {e}")

    def build_xv6(self):
        try:
            run(["make", "clean"], check=False)
            run(["make", "kernel/kernel"], check=True)
        except subprocess.CalledProcessError as e:
            raise Exception(f"Build failed: {e}")

    def cmd(self, c):
        if isinstance(c, str):
            c = c.encode("utf-8")
        self.proc.stdin.write(c)
        self.proc.stdin.flush()

    def read(self, timeout=30):
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                buf = os.read(self.proc.stdout.fileno(), 4096)
                if buf:
                    self.outbytes.extend(buf)
                    self.output = self.outbytes.decode("utf-8", "replace")
                else:
                    time.sleep(0.1)
            except BlockingIOError:
                time.sleep(0.1)
        return self.output

    def stop(self):
        try:
            self.proc.terminate()
            self.proc.wait(timeout=5)
        except:
            try:
                self.proc.kill()
            except:
                pass


def run_basic_tests():
    """Run basic system tests"""
    results = []

    print("\n" + "=" * 60)
    print("Running Basic System Tests")
    print("=" * 60)

    tests = [
        ("echo", "echo hello world\n", "hello world"),
        ("cat", "cat README\n", "xv6"),
        ("ls", "ls\n", "."),
        ("mkdir", "mkdir testdir\n", ""),
    ]

    for test_name, command, expected in tests:
        print(f"\n[TEST] {test_name}...")
        result = TestResult(test_name, "SKIP")
        start_time = time.time()

        try:
            q = QEMUTest(reset=(test_name == tests[0][0]))
            q.read(timeout=2)
            q.cmd(command)
            time.sleep(1)
            output = q.read(timeout=5)
            q.stop()

            result.duration = time.time() - start_time

            if expected == "" or expected in output:
                result.status = "PASS"
                result.output = output[-500:]  # Last 500 chars
                print(f"  ✓ PASS")
            else:
                result.status = "FAIL"
                result.output = output[-500:]
                print(f"  ✗ FAIL - Expected '{expected}' not found")
        except Exception as e:
            result.status = "ERROR"
            result.error = str(e)
            print(f"  ✗ ERROR - {e}")

        results.append(result)

    return results


def run_mlfq_test():
    """Run MLFQ scheduler test"""
    print("\n" + "=" * 60)
    print("Running MLFQ Scheduler Test")
    print("=" * 60)

    result = TestResult("mlfqtest", "SKIP")
    start_time = time.time()

    try:
        q = QEMUTest(reset=True)
        q.read(timeout=2)

        print("\n[TEST] mlfqtest...")
        q.cmd("mlfqtest\n")

        # Wait for test to complete (it has multiple phases)
        timeout = 120  # 2 minutes should be enough
        deadline = time.time() + timeout

        while time.time() < deadline:
            time.sleep(2)
            output = q.read(timeout=1)

            # Check for completion
            if "All MLFQ Tests Completed!" in output:
                result.status = "PASS"
                result.output = output
                result.duration = time.time() - start_time
                print("  ✓ PASS - MLFQ tests completed successfully")
                break

            # Check for errors
            if "panic" in output.lower() or "error" in output.lower():
                if "All MLFQ Tests Completed!" not in output:
                    result.status = "FAIL"
                    result.output = output
                    result.duration = time.time() - start_time
                    print("  ✗ FAIL - Error detected in output")
                    break

        if result.status == "SKIP":
            result.status = "FAIL"
            result.error = "Test timed out or did not complete"
            print("  ✗ FAIL - Test timed out")

        q.stop()

    except Exception as e:
        result.status = "ERROR"
        result.error = str(e)
        result.duration = time.time() - start_time
        print(f"  ✗ ERROR - {e}")

    return result


def run_usertests():
    """Run comprehensive user tests"""
    print("\n" + "=" * 60)
    print("Running User Tests (usertests)")
    print("=" * 60)

    result = TestResult("usertests", "SKIP")
    start_time = time.time()

    try:
        # Use the existing test script
        proc = run(
            ["python3", "test-xv6.py", "usertests", "-q"],
            capture_output=True,
            text=True,
            timeout=600,
        )

        result.duration = time.time() - start_time
        result.output = proc.stdout + proc.stderr

        if proc.returncode == 0 and "ALL TESTS PASSED" in proc.stdout:
            result.status = "PASS"
            print("  ✓ PASS - All user tests passed")
        else:
            result.status = "FAIL"
            result.error = f"Exit code: {proc.returncode}"
            print(f"  ✗ FAIL - Exit code: {proc.returncode}")

    except subprocess.TimeoutExpired:
        result.status = "FAIL"
        result.error = "Test timed out after 10 minutes"
        print("  ✗ FAIL - Test timed out")
    except Exception as e:
        result.status = "ERROR"
        result.error = str(e)
        print(f"  ✗ ERROR - {e}")

    return result


def generate_report(results, output_file="mlfq_test_report.txt"):
    """Generate a comprehensive test report"""
    report = []
    report.append("=" * 80)
    report.append("MLFQ Scheduler Test Report")
    report.append("=" * 80)
    report.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    report.append("")

    # Summary
    total = len(results)
    passed = sum(1 for r in results if r.status == "PASS")
    failed = sum(1 for r in results if r.status == "FAIL")
    errors = sum(1 for r in results if r.status == "ERROR")
    skipped = sum(1 for r in results if r.status == "SKIP")

    report.append("SUMMARY")
    report.append("-" * 80)
    report.append(f"Total Tests:  {total}")
    report.append(f"Passed:      {passed} ✓")
    report.append(f"Failed:      {failed} ✗")
    report.append(f"Errors:      {errors} ⚠")
    report.append(f"Skipped:     {skipped} ⊘")
    report.append(f"Success Rate: {(passed/total*100) if total > 0 else 0:.1f}%")
    report.append("")

    # Detailed Results
    report.append("DETAILED RESULTS")
    report.append("-" * 80)

    for result in results:
        status_symbol = {"PASS": "✓", "FAIL": "✗", "ERROR": "⚠", "SKIP": "⊘"}.get(
            result.status, "?"
        )

        report.append(f"\n[{status_symbol}] {result.name}")
        report.append(f"  Status: {result.status}")
        if result.duration > 0:
            report.append(f"  Duration: {result.duration:.2f}s")

        if result.error:
            report.append(f"  Error: {result.error}")

        if result.output and len(result.output) > 0:
            # Show last 20 lines of output
            lines = result.output.split("\n")
            if len(lines) > 20:
                report.append(f"  Output (last 20 lines):")
                for line in lines[-20:]:
                    report.append(f"    {line}")
            else:
                report.append(f"  Output:")
                for line in lines:
                    report.append(f"    {line}")

    report.append("")
    report.append("=" * 80)
    report.append("END OF REPORT")
    report.append("=" * 80)

    # Write to file
    report_text = "\n".join(report)
    with open(output_file, "w") as f:
        f.write(report_text)

    # Also print to console
    print("\n" + report_text)

    return report_text


def main():
    parser = argparse.ArgumentParser(description="Test MLFQ scheduler implementation")
    parser.add_argument("--mlfq-only", action="store_true", help="Run only MLFQ test")
    parser.add_argument(
        "--basic-only", action="store_true", help="Run only basic tests"
    )
    parser.add_argument(
        "--report",
        default="mlfq_test_report.txt",
        help="Output report file (default: mlfq_test_report.txt)",
    )
    args = parser.parse_args()

    all_results = []

    try:
        if not args.mlfq_only:
            # Run basic tests
            basic_results = run_basic_tests()
            all_results.extend(basic_results)

        if not args.basic_only:
            # Run MLFQ test
            mlfq_result = run_mlfq_test()
            all_results.append(mlfq_result)

            # Optionally run usertests (takes longer)
            if not args.mlfq_only:
                print("\n" + "=" * 60)
                response = input(
                    "Run comprehensive usertests? (takes ~5-10 minutes) [y/N]: "
                )
                if response.lower() == "y":
                    usertest_result = run_usertests()
                    all_results.append(usertest_result)

        # Generate report
        generate_report(all_results, args.report)
        print(f"\nReport saved to: {args.report}")

        # Exit with appropriate code
        failed_count = sum(1 for r in all_results if r.status in ["FAIL", "ERROR"])
        sys.exit(0 if failed_count == 0 else 1)

    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\nFatal error: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
