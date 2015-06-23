#include <gba_dma.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

#include <stdio.h>
#include <string.h>

#include "font.h"

struct TestTimings {
	s32 arm_text_0000;
	s32 arm_text_4000;
	s32 arm_text_0004;
	s32 arm_text_4004;
	s32 arm_text_0010;
	s32 arm_text_4010;
	s32 arm_text_0014;
	s32 arm_text_4014;
	s32 arm_ewram;
	s32 arm_iwram;
	s32 thumb_text_0000;
	s32 thumb_text_4000;
	s32 thumb_text_0004;
	s32 thumb_text_4004;
	s32 thumb_text_0010;
	s32 thumb_text_4010;
	s32 thumb_text_0014;
	s32 thumb_text_4014;
	s32 thumb_ewram;
	s32 thumb_iwram;
};

void calibrate(struct TestTimings*);
void testNop(struct TestTimings*);
void testNop2(struct TestTimings*);
void testLdrh(struct TestTimings*);
void testLdrhNop(struct TestTimings*);
void testNopLdrh(struct TestTimings*);
void testStrh(struct TestTimings*);
void testStrhNop(struct TestTimings*);
void testNopStrh(struct TestTimings*);
void testLdmia1(struct TestTimings*);
void testLdmia2(struct TestTimings*);
void testLdmia6(struct TestTimings*);
void testLdmia1x2(struct TestTimings*);
void testLdmia2x2(struct TestTimings*);
void testLdmia6x2(struct TestTimings*);
void testStmia1(struct TestTimings*);
void testStmia2(struct TestTimings*);
void testStmia6(struct TestTimings*);
void testStmia1x2(struct TestTimings*);
void testStmia2x2(struct TestTimings*);
void testStmia6x2(struct TestTimings*);
void testMul0(struct TestTimings*);
void testMul1(struct TestTimings*);
void testMul2(struct TestTimings*);
void testMul3(struct TestTimings*);
void testMul4(struct TestTimings*);
void testB(struct TestTimings*);
void testNopB(struct TestTimings*);
void testBx(struct TestTimings*);
void testDiv(struct TestTimings*);
void testCpuSet(struct TestTimings*);

#define TEST_ARM 1
#define TEST_THUMB 2
#define VIEW_SIZE 16

void longjump() {
}

struct TimingTest {
	const char* testName;
	void (*test)(struct TestTimings*);
	int modes;
	struct TestTimings expected;
} const tests[] = {
	{ "Calibration", 0, TEST_ARM | TEST_THUMB, {
		7, 4, 6, 4, 6, 2, 5, 2,
		5, 0,
		4, 1, 3, 1, 4, 0, 3, 0,
		2, 0
	} },
	{ "nop", testNop, TEST_ARM | TEST_THUMB, {
		6, 6, 6, 6, 4, 4, 4, 4,
		6, 1,
		3, 3, 3, 3, 2, 2, 2, 2,
		3, 1
	} },
	{ "nop / nop", testNop2, TEST_ARM | TEST_THUMB, {
		12, 12, 12, 12, 8, 8, 8, 8,
		12, 2,
		6, 6, 6, 6, 4, 4, 4, 4,
		6, 2
	} },
	{ "ldrh r2, [sp]", testLdrh, TEST_ARM | TEST_THUMB, {
		10, 6, 9, 6, 9, 4, 8, 4,
		8, 3,
		7, 3, 6, 3, 7, 3, 6, 3,
		5, 3
	} },
	{ "ldrh r2, [sp] / nop", testLdrhNop, TEST_ARM | TEST_THUMB, {
		16, 12, 15, 12, 13, 8, 12, 8,
		14, 4,
		10, 6, 9, 6, 9, 4, 8, 4,
		8, 4
	} },
	{ "nop / ldrh r2, [sp]", testNopLdrh, TEST_ARM | TEST_THUMB, {
		16, 12, 15, 12, 13, 8, 12, 8,
		14, 4,
		10, 6, 9, 6, 9, 5, 8, 5,
		8, 4
	} },
	{ "strh r3, [sp]", testStrh, TEST_ARM | TEST_THUMB, {
		9, 6, 8, 6, 8, 4, 7, 4,
		7, 2,
		6, 3, 5, 3, 6, 2, 5, 2,
		4, 2
	} },
	{ "strh r3, [sp] / nop", testStrhNop, TEST_ARM | TEST_THUMB, {
		15, 12, 14, 12, 12, 8, 11, 8,
		13, 3,
		9, 6, 8, 6, 8, 4, 7, 4,
		7, 3
	} },
	{ "nop / strh r3, [sp]", testNopStrh, TEST_ARM | TEST_THUMB, {
		15, 12, 14, 12, 12, 8, 11, 8,
		13, 3,
		9, 6, 8, 6, 8, 4, 7, 4,
		7, 3
	} },
	{ "ldmia sp, {r2}", testLdmia1, TEST_ARM, {
		10, 6, 9, 6, 9, 4, 8, 4,
		8, 3
	} },
	{ "ldmia sp, {r2, r3}", testLdmia2, TEST_ARM, {
		11, 6, 10, 6, 10, 4, 9, 4,
		9, 4
	} },
	{ "ldmia sp, {r2-r7}", testLdmia6, TEST_ARM, {
		15, 8, 14, 8, 14, 8, 13, 8,
		13, 8
	} },
	{ "ldmia sp, {r2} x2", testLdmia1x2, TEST_ARM, {
		20, 12, 18, 12, 18, 8, 16, 8,
		16, 6
	} },
	{ "ldmia sp, {r2, r3} x2", testLdmia2x2, TEST_ARM, {
		22, 12, 20, 12, 20, 8, 18, 8,
		18, 8
	} },
	{ "ldmia sp, {r2-r7} x2", testLdmia6x2, TEST_ARM, {
		30, 16, 28, 16, 28, 16, 26, 16,
		26, 16
	} },
	{ "stmia sp, {r2}", testStmia1, TEST_ARM, {
		9, 6, 8, 6, 8, 4, 7, 4,
		7, 2
	} },
	{ "stmia sp, {r2, r3}", testStmia2, TEST_ARM, {
		10, 6, 9, 6, 9, 4, 8, 4,
		8, 3
	} },
	{ "stmia sp, {r2-r7}", testStmia6, TEST_ARM, {
		14, 7, 13, 7, 13, 7, 12, 7,
		12, 7
	} },
	{ "stmia sp, {r2} x2", testStmia1x2, TEST_ARM, {
		18, 12, 16, 12, 16, 8, 14, 8,
		14, 4
	} },
	{ "stmia sp, {r2, r3} x2", testStmia2x2, TEST_ARM, {
		20, 12, 18, 12, 18, 8, 16, 8,
		16, 6
	} },
	{ "stmia sp, {r2-r7} x2", testStmia6x2, TEST_ARM, {
		28, 14, 26, 14, 26, 14, 24, 14,
		24, 14
	} },
	{ "mul #0x00000000, #0xFF", testMul0, TEST_ARM | TEST_THUMB, {
		9, 6, 8, 6, 8, 4, 7, 4,
		7, 2,
		6, 3, 5, 3, 6, 2, 5, 2,
		4, 2
	} },
	{ "mul #0x00000078, #0xFF", testMul1, TEST_ARM | TEST_THUMB, {
		9, 6, 8, 6, 8, 4, 7, 4,
		7, 2,
		6, 3, 5, 3, 6, 2, 5, 2,
		4, 2
	} },
	{ "mul #0x00005678, #0xFF", testMul2, TEST_ARM | TEST_THUMB, {
		10, 6, 9, 6, 9, 4, 8, 4,
		8, 3,
		7, 3, 6, 3, 7, 3, 6, 3,
		5, 3
	} },
	{ "mul #0x00345678, #0xFF", testMul3, TEST_ARM | TEST_THUMB, {
		11, 6, 10, 6, 10, 4, 9, 4,
		9, 4,
		8, 4, 7, 4, 8, 4, 7, 4,
		6, 4
	} },
	{ "mul #0x12345678, #0xFF", testMul4, TEST_ARM | TEST_THUMB, {
		12, 6, 11, 6, 11, 5, 10, 5,
		10, 5,
		9, 5, 8, 5, 9, 5, 8, 5,
		7, 5
	} },
	{ "b pc", testB, TEST_ARM | TEST_THUMB, {
		26, 26, 25, 25, 19, 19, 18, 18,
		24, 4,
		14, 14, 13, 13, 11, 11, 10, 10,
		12, 4
	} },
	{ "nop ; b pc", testB, TEST_ARM | TEST_THUMB, {
		26, 26, 25, 25, 19, 19, 18, 18,
		24, 4,
		14, 14, 13, 13, 11, 11, 10, 10,
		12, 4
	} },
	{ "bx", testBx, TEST_ARM | TEST_THUMB, {
		78, 78, 74, 74, 59, 59, 55, 55,
		72, 22,
		57, 57, 53, 53, 45, 45, 41, 41,
		51, 24
	} },
	{ "Division", testDiv, TEST_ARM | TEST_THUMB, {
		398, 398, 394, 394, 381, 381, 377, 377,
		390, 338,
		371, 371, 367, 367, 363, 363, 359, 359,
		363, 338
	} },
	{ "CpuSet", testCpuSet, TEST_ARM | TEST_THUMB, {
		3453, 3453, 3451, 3451, 3434, 3434, 3432, 3432,
		3449, 3397,
		3456, 3456, 3448, 3448, 3447, 3447, 3439, 3439,
		3440, 3403
	} },
};

u16* textBase = (u16*) VRAM;
char textGrid[32 * 32];

#define REG_WAITCNT (*(vu16*) 0x4000204)

EWRAM_DATA const int _anchor = 0xABAD1DEA; // There seems to be a bug in the ld script that this fixes

static void updateTextGrid(void) {
	int i;
	for (i = 0; i < 20 * 32; ++i) {
		textBase[i] = textGrid[i] ? textGrid[i] - ' ' : 0;
	}
}

static void printResult(int offset, int line, const char* preface, s32 value, s32 calibration, s32 expected) {
	static const int base = 96;
	if (offset > line || base + 32 * (line - offset) > 576) {
		return;
	}

	snprintf(&textGrid[base + 32 * (line - offset)], 31, "%-13s: %5i", preface, value - calibration);
	if (value - calibration == expected) {
		strncpy(&textGrid[base + 32 * (line - offset) + 21], "PASS", 10);
	} else {
		snprintf(&textGrid[base + 32 * (line - offset) + 21], 10, "!= %5i", expected);
	}
}

static void printResults(const char* preface, const struct TestTimings* values, const struct TestTimings* calibration, const struct TestTimings* expected, int mode, int base) {
	snprintf(&textGrid[32], 31, "Timing test: %s", preface);

	printResult(base, 0, "ARM/ROM ...", values->arm_text_0000, calibration->arm_text_0000, expected->arm_text_0000);
	printResult(base, 1, "ARM/ROM P..", values->arm_text_4000, calibration->arm_text_4000, expected->arm_text_4000);
	printResult(base, 2, "ARM/ROM .N.", values->arm_text_0004, calibration->arm_text_0004, expected->arm_text_0004);
	printResult(base, 3, "ARM/ROM PN.", values->arm_text_4004, calibration->arm_text_4004, expected->arm_text_4004);
	printResult(base, 4, "ARM/ROM ..S", values->arm_text_0010, calibration->arm_text_0010, expected->arm_text_0010);
	printResult(base, 5, "ARM/ROM P.S", values->arm_text_4010, calibration->arm_text_4010, expected->arm_text_4010);
	printResult(base, 6, "ARM/ROM .NS", values->arm_text_0014, calibration->arm_text_0014, expected->arm_text_0014);
	printResult(base, 7, "ARM/ROM PNS", values->arm_text_4014, calibration->arm_text_4014, expected->arm_text_4014);
	printResult(base, 8, "ARM/WRAM", values->arm_ewram, calibration->arm_ewram, expected->arm_ewram);
	printResult(base, 9, "ARM/IWRAM", values->arm_iwram, calibration->arm_iwram, expected->arm_iwram);

	if (mode & TEST_THUMB) {
		printResult(base, 10, "Thumb/ROM ...", values->thumb_text_0000, calibration->thumb_text_0000, expected->thumb_text_0000);
		printResult(base, 11, "Thumb/ROM P..", values->thumb_text_4000, calibration->thumb_text_4000, expected->thumb_text_4000);
		printResult(base, 12, "Thumb/ROM .N.", values->thumb_text_0004, calibration->thumb_text_0004, expected->thumb_text_0004);
		printResult(base, 13, "Thumb/ROM PN.", values->thumb_text_4004, calibration->thumb_text_4004, expected->thumb_text_4004);
		printResult(base, 14, "Thumb/ROM ..S", values->thumb_text_0010, calibration->thumb_text_0010, expected->thumb_text_0010);
		printResult(base, 15, "Thumb/ROM P.S", values->thumb_text_4010, calibration->thumb_text_4010, expected->thumb_text_4010);
		printResult(base, 16, "Thumb/ROM .NS", values->thumb_text_0014, calibration->thumb_text_0014, expected->thumb_text_0014);
		printResult(base, 17, "Thumb/ROM PNS", values->thumb_text_4014, calibration->thumb_text_4014, expected->thumb_text_4014);
		printResult(base, 18, "Thumb/WRAM", values->thumb_ewram, calibration->thumb_ewram, expected->thumb_ewram);
		printResult(base, 19, "Thumb/IWRAM", values->thumb_iwram, calibration->thumb_iwram, expected->thumb_iwram);
	}
}

int main(void) {
	irqInit();

	BG_PALETTE[0] = 0x7FFF;
	DMA3COPY(fontTiles, TILE_BASE_ADR(1), DMA16 | DMA_IMMEDIATE | (fontTilesLen >> 1));
	REG_BG1CNT = CHAR_BASE(1) | SCREEN_BASE(0);
	REG_BG1VOFS = -4;
	memset(textGrid, 0, sizeof(textGrid));
	strcpy(&textGrid[2], "Game Boy Advance Test Suite");

	strcpy(&textGrid[139], "Testing...");
	updateTextGrid();
	REG_DISPCNT = MODE_0 | BG1_ON;

	irqEnable(IRQ_VBLANK);

	const struct TimingTest* activeTest = 0;
	int testIndex = 0;
	int viewIndex = 0;
	int resultIndex = 0;

	struct TestTimings calibration;

	int passes = 0;
	int totalResults = 0;
	int i;
	for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i) {
		struct TestTimings currentTest = {0};
		VBlankIntrWait();
		REG_IME = 0;
		calibrate(&calibration);
		activeTest = &tests[i];
		if (activeTest->test) {
			activeTest->test(&currentTest);
		} else {
			currentTest = calibration;
			memset(&calibration, 0, sizeof(calibration));
		}
		REG_IME = 1;
		if (activeTest->expected.arm_text_0000 == currentTest.arm_text_0000 - calibration.arm_text_0000) {
			++passes;
		}
		if (activeTest->expected.arm_text_4000 == currentTest.arm_text_4000 - calibration.arm_text_4000) {
			++passes;
		}
		if (activeTest->expected.arm_text_0004 == currentTest.arm_text_0004 - calibration.arm_text_0004) {
			++passes;
		}
		if (activeTest->expected.arm_text_4004 == currentTest.arm_text_4004 - calibration.arm_text_4004) {
			++passes;
		}
		if (activeTest->expected.arm_text_0010 == currentTest.arm_text_0010 - calibration.arm_text_0010) {
			++passes;
		}
		if (activeTest->expected.arm_text_4010 == currentTest.arm_text_4010 - calibration.arm_text_4010) {
			++passes;
		}
		if (activeTest->expected.arm_text_0014 == currentTest.arm_text_0014 - calibration.arm_text_0014) {
			++passes;
		}
		if (activeTest->expected.arm_text_4014 == currentTest.arm_text_4014 - calibration.arm_text_4014) {
			++passes;
		}
		if (activeTest->expected.arm_ewram == currentTest.arm_ewram - calibration.arm_ewram) {
			++passes;
		}
		if (activeTest->expected.arm_iwram == currentTest.arm_iwram - calibration.arm_iwram) {
			++passes;
		}
		totalResults += 10;
		if (activeTest->modes & TEST_THUMB) {
			if (activeTest->expected.thumb_text_0000 == currentTest.thumb_text_0000 - calibration.thumb_text_0000) {
				++passes;
			}
			if (activeTest->expected.thumb_text_4000 == currentTest.thumb_text_4000 - calibration.thumb_text_4000) {
				++passes;
			}
			if (activeTest->expected.thumb_text_0004 == currentTest.thumb_text_0004 - calibration.thumb_text_0004) {
				++passes;
			}
			if (activeTest->expected.thumb_text_4004 == currentTest.thumb_text_4004 - calibration.thumb_text_4004) {
				++passes;
			}
			if (activeTest->expected.thumb_text_0010 == currentTest.thumb_text_0010 - calibration.thumb_text_0010) {
				++passes;
			}
			if (activeTest->expected.thumb_text_4010 == currentTest.thumb_text_4010 - calibration.thumb_text_4010) {
				++passes;
			}
			if (activeTest->expected.thumb_text_0014 == currentTest.thumb_text_0014 - calibration.thumb_text_0014) {
				++passes;
			}
			if (activeTest->expected.thumb_text_4014 == currentTest.thumb_text_4014 - calibration.thumb_text_4014) {
				++passes;
			}
			if (activeTest->expected.thumb_ewram == currentTest.thumb_ewram - calibration.thumb_ewram) {
				++passes;
			}
			if (activeTest->expected.thumb_iwram == currentTest.thumb_iwram - calibration.thumb_iwram) {
				++passes;
			}
			totalResults += 10;
		}
	}

	activeTest = 0;
	while (1) {
		memset(&textGrid[32], 0, sizeof(textGrid) - 32);
		scanKeys();
		u16 keys = keysDown();

		if (keys & KEY_B) {
			activeTest = 0;
			resultIndex = 0;
		}
		if (keys & KEY_A) {
			activeTest = &tests[testIndex];
		}

		if (activeTest) {
			struct TestTimings currentTest = {0};
			if (keys & KEY_UP) {
				--resultIndex;
				if (resultIndex < 0) {
					resultIndex = 4;
				}
			}
			if (keys & KEY_DOWN) {
				++resultIndex;
				resultIndex %= 5;
			}
			if (activeTest->test) {
				activeTest->test(&currentTest);
				printResults(activeTest->testName, &currentTest, &calibration, &activeTest->expected, activeTest->modes, resultIndex);
			} else {
				printResults(activeTest->testName, &calibration, &currentTest, &activeTest->expected, activeTest->modes, resultIndex);
			}
		} else {
			if (keys & KEY_UP) {
				--testIndex;
				if (testIndex < 0) {
					testIndex = sizeof(tests) / sizeof(*tests) - 1;
				}
			}
			if (keys & KEY_DOWN) {
				++testIndex;
				testIndex %= sizeof(tests) / sizeof(*tests);
			}
			if (testIndex < viewIndex) {
				viewIndex = testIndex;
			} else if (testIndex >= viewIndex + VIEW_SIZE) {
				viewIndex = testIndex - VIEW_SIZE + 1;
			}
			strcpy(&textGrid[32], "Timing tests");
			sprintf(&textGrid[55], "%3u/%-3u", passes, totalResults);
			size_t i;
			for (i = 0; i < sizeof(tests) / sizeof(*tests) && i < VIEW_SIZE; ++i) {
				snprintf(&textGrid[96 + i * 32], 31, "%c%s", (i + viewIndex == testIndex) ? '>' : ' ', tests[i + viewIndex].testName);
			}
		}

		updateTextGrid();
		VBlankIntrWait();
	}

	return 0;
}
