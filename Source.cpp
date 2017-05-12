#include <algorithm>
#include <fstream>
#include <iostream>
#include <ql\quantlib.hpp>
#include <sstream>
#include <string>
#include <vector>
using namespace QuantLib;

// Set variables
void setValue() {
	
}

// Construct volatility smile
boost::shared_ptr<BlackVolTermStructure> bootstrapVolatilityCurve(
	Date& evaluationDate, Date& expirationDate, Real dK) {
	// Calendar for volatility term-structure
	Calendar calendarVol = UnitedStates(UnitedStates::NYSE);

	// Store expirationDate into a vector
	std::vector<Date> expirationDates = { expirationDate };

	// Strikes (put 50~100, call 100~135)
	std::vector<Real> strikes;
	for (Real strike = 50; strike <= 135; strike += dK) {
		strikes.emplace_back(strike);
	}

	// Volatility (put 0.3~0.2, call 0.2~0.13)
	// BlackVarianceSurface() function only accepts Matrix-type vols
	Matrix vols(strikes.size(), 1);
	for (int i = 0; i < strikes.size(); i++) {
		vols[i][0] = (Real)(0.3 - 0.002*dK*i);
	}

	// Construct volatility term-structure
	boost::shared_ptr<BlackVolTermStructure> volCurve(new
		BlackVarianceSurface(evaluationDate, calendarVol, expirationDates,
			strikes, vols, Actual365Fixed()
		)
	);

	return volCurve;
}

// Bootstrap LIBOR zero-rate curve
/*boost::shared_ptr<YieldTermStructure> bootstrapLiborZeroCurve(Date& evaluationDate) {
	// Step 1: Calendar for interest rate term structure
	Calendar calendarLIBOR = UnitedStates(UnitedStates::NYSE);
	Settings::instance().evaluationDate() = evaluationDate;

	// Step 2: Day counter
	DayCounter dayCounterLIBOR = Actual365Fixed();

	// Step 3: Construct interest rate & dates vectors
	// Step 3.1: Read ir schedule from file
	std::ifstream inputFile("ir_schedule.csv");
	if (!inputFile.is_open()) {
		std::exit(EXIT_FAILURE);
	}
	// Step 3.2: Skip 1st line of title
	std::string line;
	//// Skip 1st line
	std::getline(inputFile, line);

	// Step 3.3: Read 2nd - last lines
	std::vector<Date> irDates;
	std::vector<Rate> irRates;
	while (std::getline(inputFile, line)) {
		std::istringstream iss{ line };
		std::vector<std::string> tokens;
		std::string token;
		while (std::getline(iss, token, ',')) {
			tokens.emplace_back(token);
		}
		Date irDate = DateParser::parseFormatted(tokens[0], "%Y-%m-%d");
		Rate irRate = std::stod(tokens[1]);
		irDates.emplace_back(irDate);
		irRates.emplace_back(irRate);
	}

	// Step 4: Build LIBOR term structure
	boost::shared_ptr<YieldTermStructure> irCurve(new
		InterpolatedZeroCurve<ForwardFlat>(irDates, irRates, dayCounterLIBOR, calendarLIBOR, ForwardFlat())
	);

	return irCurve;
}*/
boost::shared_ptr<YieldTermStructure> bootstrapLiborZeroCurve(Date& evaluationDate) {
	std::vector<Date> irDates = { evaluationDate, evaluationDate + 1 * Days,
		evaluationDate + 1 * Weeks, evaluationDate + 1 * Months, evaluationDate + 2 * Months,
		evaluationDate + 3 * Months, evaluationDate + 4 * Months, evaluationDate + 7 * Months,
		evaluationDate + 10 * Months, evaluationDate + 13 * Months, evaluationDate + 16 * Months,
		evaluationDate + 19 * Months, evaluationDate + 22 * Months, evaluationDate + 25 * Months,
		evaluationDate + 28 * Months, evaluationDate + 31 * Months, evaluationDate + 34 * Months,
		evaluationDate + 37 * Months, evaluationDate + 40 * Months, evaluationDate + 43 * Months,
		evaluationDate + 46 * Months, evaluationDate + 4 * Years, evaluationDate + 5 * Years,
		evaluationDate + 6 * Years, evaluationDate + 7 * Years, evaluationDate + 8 * Years,
		evaluationDate + 9 * Years, evaluationDate + 10 * Years, evaluationDate + 11 * Years,
		evaluationDate + 12 * Years, evaluationDate + 13 * Years, evaluationDate + 14 * Years };
	std::vector<Rate> irRates;
	for (size_t i = 0; i < irDates.size(); i++) {
		irRates.emplace_back(0.05);
	}

	DayCounter dayCounterLIBOR = Actual365Fixed();
	Calendar calendarLIBOR = Calendar();

	// Construct LIBOR term-structure
	boost::shared_ptr<YieldTermStructure> irCurve(new
		InterpolatedZeroCurve<ForwardFlat>(irDates, irRates, dayCounterLIBOR, calendarLIBOR, ForwardFlat())
	);

	return irCurve;
}

// Bootstrap dividend curve
boost::shared_ptr<ZeroCurve> bootstrapDividendCurve(
	Date& evaluationDate, Date& expirationDate, Real spot) {
	// Step 1: Calendar for dividend term structure
	Calendar calendarDiv = UnitedStates(UnitedStates::NYSE);
	Settings::instance().evaluationDate() = evaluationDate;

	// Step 2: Day counter & Year fraction
	DayCounter dayCounterDiv = ActualActual();
	Real settlementDays = 2;
	Date settlementDate = expirationDate + settlementDays;
	Real yearFractionDiv = dayCounterDiv.yearFraction(evaluationDate, settlementDate);

	// Step 3: Construct dividend yields & dates vectors
	// Step 3.1: Read dividend schedule from file
	std::ifstream inputFile("dividend_schedule.csv");
	if (!inputFile.is_open()) {
		std::exit(EXIT_FAILURE);
	}
	// Step 3.2: Skip 1st line of title
	std::string line;
	//// Skip 1st line
	std::getline(inputFile, line);

	// Step 3.3: Read 2nd - last lines
	std::vector<Date> divDates;
	std::vector<Rate> divYields;
	while (std::getline(inputFile, line)) {
		std::istringstream iss{ line };
		std::vector<std::string> tokens;
		std::string token;
		while (std::getline(iss, token, ',')) {
			tokens.emplace_back(token);
		}
		Date divDate = DateParser::parseFormatted(tokens[0], "%Y-%m-%d");
		Rate divYield = std::stod(tokens[1]) * yearFractionDiv / spot;
		divDates.emplace_back(divDate);
		divYields.emplace_back(divYield);
	}

	// Step 4: Build zero curve term structure
	boost::shared_ptr<ZeroCurve> divCurve(new
		ZeroCurve(divDates, divYields, ActualActual(), calendarDiv)
	);

	return divCurve;
}
/*boost::shared_ptr<YieldTermStructure> bootstrapDividendCurve(
	Date& evaluationDate, Date& expirationDate, Real spot) {
	// Day counter
	DayCounter dayCounterDiv = Actual365Fixed();

	// Construct dividend term-structure
	boost::shared_ptr<YieldTermStructure> divCurve(new FlatForward(evaluationDate, 0.0, dayCounterDiv));

	return divCurve;
}*/

// Main calculation and output function
void printOut() {
	// Dates
	Date evaluationDate = Date(15, April, 2017);
	Date expirationDate = Date(13, July, 2017);
	Settings::instance().evaluationDate() = evaluationDate;

	// Data from "A Guide to Volatility and Variance Swaps"
	// Derman, Kamal & Zou, 1999
	Position::Type type = Position::Type::Long;
	Real varStrike = 0.01;
	Real nominal = 50000;
	Real spot = 100.0;

	// dK affects results a lot
	Real dK = 2.5;

	// Used for ReplicatingVarianceSwapEngine function
	std::vector<Real> callStrikes;
	for (Real callStrike = 100; callStrike <= 135; callStrike += dK) {
		callStrikes.emplace_back(callStrike);
	}
	std::vector<Real> putStrikes;
	for (Real putStrike = 50; putStrike <= 100; putStrike += dK) {
		putStrikes.emplace_back(putStrike);
	}

	// Numerix-calculated result
	Real numerix = 0.040694157;

	// Step 1: Deal with volatility
	// Step 1.1: build volatility term-structure
	boost::shared_ptr<BlackVolTermStructure> volTS = bootstrapVolatilityCurve(evaluationDate, expirationDate, dK);
	// Step 1.2: store volTS into handle
	Handle<BlackVolTermStructure> volTSHandle = Handle<BlackVolTermStructure>(volTS);

	// Step 2: Deal with interest rate
	// Step 2.1: build LIBOR term-structure
	boost::shared_ptr<YieldTermStructure> irTS = bootstrapLiborZeroCurve(evaluationDate);
	// Step 2.2: store irTS into handle
	Handle<YieldTermStructure> irTSHandle = Handle<YieldTermStructure>(irTS);

	// Step 3: Deal with dividend
	// Step 3.1: build dividend term-structure
	boost::shared_ptr<YieldTermStructure> divTS = bootstrapDividendCurve(evaluationDate, expirationDate, spot);
	// Step 3.2: store divTS into handle
	Handle<YieldTermStructure> divTSHandle = Handle<YieldTermStructure>(divTS);

	// Step 4: Deal with quote price
	// Step 4.1: build simple quote
	boost::shared_ptr<Quote> spotQuote = boost::shared_ptr<Quote>(new SimpleQuote(spot));
	// Step 4.2: store spotQuote into handle
	Handle<Quote> spotQuoteHandle = Handle<Quote>(spotQuote);

	// Build BlackScholesMertonProcess()
	boost::shared_ptr<GeneralizedBlackScholesProcess> stochProcess(new
		BlackScholesMertonProcess(spotQuoteHandle, divTSHandle, irTSHandle, volTSHandle)
	);

	// Build Pricing Engine
	boost::shared_ptr<PricingEngine> engine(new
		ReplicatingVarianceSwapEngine(
			stochProcess,	// BlackScholesMertonProcess
			dK,				// dK affects results a lot
			callStrikes,
			putStrikes)
	);

	// Call VarianceSwap class, then Set Pricing Engine
	VarianceSwap varianceSwap(type, varStrike, nominal, evaluationDate, expirationDate);
	varianceSwap.setPricingEngine(engine);

	// Output
	Real calculated = varianceSwap.variance();
	Real error = std::fabs(calculated - numerix);
	std::cout << "Numerix: " << numerix << ", Calculated: " << calculated
		<< ", Error: " << error << std::endl;
}

int main() {
	printOut();

	system("pause");
	return 0;
}