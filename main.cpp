// =============================================================================
// Lab 12: Extracting double-precision numbers from strings
// =============================================================================
// Author: Jaycob
//
//
// AI assistance (Claude):
//   - Helped diagnose bugs by running test suites against my code and
//     identifying root causes (e.g. the 'e' in words like "temp"/"value"
//     being mistaken for an exponent marker, sign-grab incorrectly firing
//     on the exponent's '-' sign, off-by-one in the digit-accumulation loop).
//   - Suggested the inNumber / finishedNumber / seenExp state flags to fix
//     the "letters in number" and "trailing garbage" cases.
//   - Suggested the post-loop validation passes (digit count check, decimal
//     position check, exponent digit check) and the overflow handling.
//   - Did not write the original parser logic (preDecimal/postDecimal
//     accumulation, applyExponent, characterToDouble) — those were mine.
//   - claude also helped write the comments
// =============================================================================

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
using namespace std;

// -----------------------------------------------------------------------------
// characterToDouble
//   Converts a single digit character ('0'-'9') to its numeric value.
//   Returns -1 for any non-digit character.
// -----------------------------------------------------------------------------
double characterToDouble(char character) {
    switch (character) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default:  return -1;
    }
}

// -----------------------------------------------------------------------------
// applyExponent
//   Multiplies (or divides) input by 10 a total of 'exponent' times.
//   Used to apply the scientific-notation exponent to the parsed base value.
//   Example: applyExponent(1.23, true, 5)  ->  123000.0
//            applyExponent(1.23, false, 4) ->  0.000123
// -----------------------------------------------------------------------------
double applyExponent(double input, bool isPositive, int exponent) {
    if (isPositive) {
        for (int i = 0; i < exponent; i++) input = input * 10;
    } else {
        for (int i = 0; i < exponent; i++) input = input / 10;
    }
    return input;
}

// -----------------------------------------------------------------------------
// collectValidChars
//   First pass of the parser. Walks the input string left-to-right and
//   pushes only the characters that belong to the embedded number into
//   the 'numbers' vector.
//
//   Validates as it goes — returns -1 for any invalid pattern. Otherwise
//   returns the number of characters collected.
//
//   State flags:
//     inNumber       - currently building the number (saw a digit or '.')
//     finishedNumber - number ended; any further digit means INVALID
//     seenExp        - already processed the exponent marker for this number
// -----------------------------------------------------------------------------
int collectValidChars(string input, vector<char> *numbersPtr) {
    vector<char>& numbers = *numbersPtr;     // alias so we can use [] / push_back cleanly
    bool gotSign = false;                    // whether the base sign was captured
    int length = 0;                          // number of chars pushed into 'numbers'
    int periodCounter = 0;                   // total decimal points seen in number
    bool inNumber = false;                   // currently inside the number
    bool finishedNumber = false;             // number has ended, no more digits allowed
    bool seenExp = false;                    // 'e'/'E' has been processed

    for (int i = 0; i < (int)input.length(); i++) {
        char c = input[i];

        // ---------------- digits ----------------
        if (isdigit(c)) {
            // A digit appearing AFTER the number already ended means the
            // input has letters/garbage between digits (e.g. "12a3.45").
            // The spec calls this invalid.
            if (finishedNumber) return -1;

            // Capture the base sign — only valid right before the FIRST digit.
            // (!gotSign && !inNumber means we haven't started the number yet.)
            if (not gotSign && !inNumber){
                if (i > 0 && input[i - 1] == '-') {
                    numbers.push_back(input[i - 1]);
                    gotSign = true;
                    length++;
                };
            }

            numbers.push_back(c);
            length++;
            inNumber = true;
            continue;
        }

        // ---------------- decimal point ----------------
        if (c == '.') {
            // A '.' after the number finished is just trailing garbage.
            if (finishedNumber) continue;
            // A '.' inside the exponent is invalid (e.g. "1.2e3.4").
            if (seenExp) return -1;

            periodCounter++;
            numbers.push_back(c);
            length++;
            inNumber = true;
            continue;
        }

        // ---------------- exponent marker ----------------
        if (c == 'e' or c == 'E') {
            // 'e' before any digit is just a letter in some word like
            // "temp" or "hello" — ignore it.
            if (!inNumber) continue;

            // If we already processed an exponent for this number, then a
            // second 'e' is part of trailing garbage like "...e+3end".
            // End the number; don't reject.
            if (seenExp) {
                finishedNumber = true;
                inNumber = false;
                continue;
            }

            // Need a following character (digit, '+', or '-') for the exponent.
            if (i + 1 >= (int)input.length()) return -1;
            char next = input[i + 1];
            if (next == '+' || next == '-' || isdigit(next)) {
                numbers.push_back(c);
                numbers.push_back(next);
                i++;                  // skip the sign/digit we just consumed
                length += 2;
                seenExp = true;
                continue;
            } else {
                return -1;            // 'e' followed by garbage like "1eX"
            }
        }

        // ---------------- any other character ----------------
        // If we were inside a number, this character ends it.
        if (inNumber) {
            finishedNumber = true;
            inNumber = false;
        }
    }

    // ===== Post-loop validation =====

    // Must have at least one digit somewhere.
    bool hasDigit = false;
    for (char ch : numbers) {
        if (isdigit(ch)) { hasDigit = true; break; }
    }
    if (!hasDigit) return -1;

    // Multiple decimal points (e.g. "12.34.56") is invalid.
    if (periodCounter > 1) return -1;

    // A double has ~17 digits of precision. More integer digits than that
    // means the value can't be represented faithfully (e.g. "9999...9999").
    int intDigits = 0;
    for (int k = 0; k < length; k++) {
        if (numbers[k] == '.' || numbers[k] == 'e' || numbers[k] == 'E') break;
        if (isdigit(numbers[k])) intDigits++;
    }
    if (intDigits > 17) return -1;

    // If we started an exponent, make sure it has at least one digit and
    // contains nothing else after its (optional) sign.
    if (seenExp) {
        int ePos = -1;
        for (int k = 0; k < length; k++) {
            if (numbers[k] == 'e' || numbers[k] == 'E') { ePos = k; break; }
        }
        int k = ePos + 1;
        if (k < length && (numbers[k] == '+' || numbers[k] == '-')) k++;
        bool expHasDigit = false;
        for (; k < length; k++) {
            if (isdigit(numbers[k])) { expHasDigit = true; break; }
            return -1;     // a non-digit appears in exponent
        }
        if (!expHasDigit) return -1;
    }

    // A lone decimal point with no digits on either side (e.g. "-.") is invalid.
    if (periodCounter == 1) {
        int dotPos = -1;
        for (int k = 0; k < length; k++) {
            if (numbers[k] == '.') { dotPos = k; break; }
        }
        bool digitBefore = false, digitAfter = false;
        for (int k = 0; k < dotPos; k++) {
            if (isdigit(numbers[k])) { digitBefore = true; break; }
        }
        for (int k = dotPos + 1; k < length; k++) {
            if (numbers[k] == 'e' || numbers[k] == 'E') break;
            if (isdigit(numbers[k])) { digitAfter = true; break; }
        }
        if (!digitBefore && !digitAfter) return -1;
    }

    return length;
}

// -----------------------------------------------------------------------------
// buildNumber
//   Second pass of the parser. Takes the already-validated character vector
//   from collectValidChars and accumulates the actual double value.
//
//   Approach:
//     - Pre-decimal digits: build by repeated multiply-by-10 and add.
//       e.g. "123" -> 0*10+1=1, 1*10+2=12, 12*10+3=123
//     - Post-decimal digits: divide each new digit by a growing divisor
//       (10, 100, 1000...) and add to a running total.
//       e.g. ".34" -> 0 + 3/10 = 0.3, 0.3 + 4/100 = 0.34
//     - Exponent digits: build like an integer, then apply via applyExponent.
// -----------------------------------------------------------------------------
double buildNumber(vector<char> *numbersPtr, int length) {
    vector<char>& numbers = *numbersPtr;
    double result = 0.0;
    double preDecimal = 0.0;             // digits before the decimal point
    double postDecimal = 0.0;            // digits after the decimal point
    int exponental = 0;                  // exponent value
    bool seenDecimal = false;            // we've passed the '.'
    bool seenExponental = false;         // we've passed the 'e'/'E'
    bool positiveExponental = true;      // sign of exponent (default +)
    int divisor = 10;                    // grows as we read each fractional digit

    for (int i = 0; i < length; i++) {
        // State transitions on '.', 'e', 'E'
        if (numbers[i] == '.'){seenDecimal = true; continue;}
        if (numbers[i] == 'e' or numbers[i] == 'E') {seenExponental = true; continue;}

        // Phase 1: pre-decimal portion of the base
        if (not seenExponental && not seenDecimal) {
            if (numbers[i] == '-') continue;       // skip the base sign char
            preDecimal = preDecimal * 10 + characterToDouble(numbers[i]);
            continue;
        }

        // Phase 2: post-decimal portion of the base
        if (seenDecimal && not seenExponental) {
            postDecimal = postDecimal + (characterToDouble(numbers[i]) / divisor);
            divisor = divisor * 10;
        }

        // Phase 3: exponent
        if (seenExponental) {
            if (numbers[i] == '+'){positiveExponental = true;  continue;}
            if (numbers[i] == '-'){positiveExponental = false; continue;}
            exponental = exponental * 10 + (int)characterToDouble(numbers[i]);
        }
    }

    // Catch huge positive exponents that would overflow a double.
    // 1.7e308 is the approximate maximum for a double.
    if (exponental > 308 && positiveExponental && (preDecimal + postDecimal) != 0) {
        return INFINITY;        // signal to extractNumeric that this overflowed
    }

    result = preDecimal + postDecimal;
    result = applyExponent(result, positiveExponental, exponental);

    // Apply the sign captured in numbers[0] during the first pass.
    if (numbers[0] == '-'){result = result * -1;}
    return result;
}

// -----------------------------------------------------------------------------
// extractNumeric
//   Public entry point as required by the spec. Coordinates the two passes
//   and converts any overflow / infinity / NaN into the sentinel -999999.99.
// -----------------------------------------------------------------------------
double extractNumeric(const string& str) {
    vector<char> numbers;
    int length = collectValidChars(str, &numbers);
    if (length == -1) return -999999.99;

    double result = buildNumber(&numbers, length);

    // Reject inf/nan/out-of-range results.
    if (isinf(result) || isnan(result)) return -999999.99;
    if (result > 1.7e308 || result < -1.7e308) return -999999.99;

    return result;
}

// -----------------------------------------------------------------------------
// printResult
//   Formats output per spec: 4 decimal places fixed-point, or the invalid msg.
// -----------------------------------------------------------------------------
void printResult(double result) {
    if (result == -999999.99) {
        cout << "Invalid input: no valid floating-point number found" << endl;
    } else {
        cout << "Extracted number: " << fixed << setprecision(4) << result << endl;
    }
}

// -----------------------------------------------------------------------------
// main
//   Repeatedly prompts the user for input until they type END.
// -----------------------------------------------------------------------------
int main() {
    string input;

    while (true) {
        cout << "Enter a string (or 'END' to quit): ";
        getline(cin, input);

        if (input == "END") break;

        double result = extractNumeric(input);
        printResult(result);
    }

    cout << "Program terminated." << endl;
    return 0;
}