# Simplified Quadratic Frobenius Primality Tester With Third Roots Of Unity

## 1. Source and implemented algorithm

The implementation follows Martin Seysen, "A Simplified Quadratic Frobenius Primality Test", IACR ePrint 2005/462.

The dedicated class implements the following paper algorithms:

1. **Algorithm MR2**: Miller-Rabin with basis two or a small quadratic nonresidue, producing the quadratic parameter `c` and a primitive eighth root of unity `epsilon`.
2. **Algorithm SQFT3round**: one strengthened Simplified Quadratic Frobenius round with the Frobenius identity, eighth-root condition, and third-root-of-unity extraction/consistency check.
3. **Algorithm SQFT3**: trial division below 200, one MR2 preparation, and the requested number of strengthened rounds.

The paper's short algorithm labels are retained only in comments where they identify the original pseudocode. C++ class and function names are written out in full.

## 2. Class ownership

```text
PrimeNumberTester
    |
    +-- SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity(...)
            |
            +-- delegates to
                SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity
                    |
                    +-- TestProbablePrimality(...)
                    +-- private modular arithmetic
                    +-- private Jacobi-symbol preparation
                    +-- private quadratic-extension ring arithmetic
                    +-- private eighth-root checks
                    +-- private third-root cross-round checks
```

`PrimeNumberTester` does not contain the quadratic-extension implementation. It owns one dedicated algorithm object and exposes only the project-level dispatch function.

## 3. Files

- `SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity.hpp`
- `SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity.cpp`
- `SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnityTest.cpp`
- `SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnitySanitizerTest.cpp`
- modified `PrimeNumberTester.hpp`
- modified `PrimeNumberTester.cpp`
- modified `CryptographyAsymmetricKey.hpp`
- modified `CryptographyAsymmetricKey.cpp`
- modified `CMakeLists.txt`

## 4. Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The default CMake build compiles and links:

- `BigNumberSystem`;
- `CryptographySymmetricKey`;
- `CryptographyAsymmetricKey`;
- `TestCryptography`;
- the complete Frobenius regression test;
- the reduced sanitizer-oriented Frobenius regression test.

The previous delivery temporarily disabled the existing cryptography targets because `CryptographyAsymmetricKey.cpp` no longer matched the current `BigInteger` interface. Those build errors are now repaired:

- `BitSize()` was updated to `BitLength()`;
- machine-word exponent arithmetic no longer subtracts a `BigInteger` constant from `size_t`;
- asynchronous workers are joined through `std::future::get()`;
- the ordinary RSA prime-candidate loop now advances by two instead of retesting one composite forever.

Only users who deliberately want the core number library without the demonstration targets need to disable them:

```bash
cmake -S . -B build -DBUILD_EXISTING_CRYPTOGRAPHY_EXAMPLE_TARGETS=OFF
```

## 5. Regression coverage

The complete regression test covers:

- every integer from 0 through 5000 against exact machine-word trial division;
- rejection of zero testing rounds;
- large composites whose prime factors are all greater than 200;
- one known large prime in each odd residue class modulo eight;
- the Mersenne prime `2^127-1`;
- the secp256k1 field prime;
- a large odd perfect square;
- agreement between the `PrimeNumberTester` dispatcher and the dedicated algorithm class.

The reduced sanitizer suite covers the same public ownership path, every MR2 residue branch, multiple complete extension-ring rounds, and a semiprime, while omitting the expensive 256-bit vector.

## 6. Validation performed in the delivery environment

- GNU C++ 14, Debug: clean configure, complete default build including both cryptography libraries and `TestCryptography`, complete Frobenius regression test, CTest.
- Clang C++ 17, Release: clean configure, complete default build including both cryptography libraries and `TestCryptography`, both CTest tests.
- GNU C++ 14 with AddressSanitizer and UndefinedBehaviorSanitizer: reduced sanitizer suite passed.

The full regression suite was also attempted under sanitizers, but the intentionally division-independent bit-by-bit modular multiplication made that instrumented run exceed the execution limit. It is not reported as passed.
