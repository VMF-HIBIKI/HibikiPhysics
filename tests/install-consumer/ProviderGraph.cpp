#include <HibikiPhysics/Plugin.h>
#include <iostream>

namespace
{
using namespace HibikiPhysics;

void Check(bool inCondition, const char *inMessage)
{
	if (!inCondition)
		throw std::logic_error(inMessage);
}

template <class Error, class Run>
void ExpectError(Run inRun, std::string_view inExpected)
{
	try
	{
		inRun();
	}
	catch (const Error &error)
	{
		if (error.what() == inExpected)
			return;
		throw;
	}
	throw std::logic_error("Expected graph error");
}

struct Node
{
	std::string &mLog;
	char		 mId;
	const Node	*mDependency;
	~Node() { mLog.push_back(mId); }
};

void CheckDiamond()
{
	std::string log;
	log.reserve(32);
	const CapabilitySlot<Node> root("root"), left("left"), right("right"), leaf("leaf");
	CapabilityConfig		   config;
	config.Add(root.Factory("root-impl", [&](Node &a, Node &b)
							{
		Check(a.mDependency == b.mDependency, "diamond must share the leaf instance");
		log += 'R';
		return std::make_unique<Node>(log, 'r', &a); }, left, right));
	config.Add(left.Factory("left-impl", [&](Node &l)
							{ log += 'A'; return std::make_unique<Node>(log, 'a', &l); }, leaf));
	config.Add(right.Factory("right-impl", [&](Node &l)
							 { log += 'B'; return std::make_unique<Node>(log, 'b', &l); }, leaf));
	config.Add(leaf.Factory("leaf-impl", [&]
							{ log += 'L'; return std::make_unique<Node>(log, 'l', nullptr); }));
	auto plugin = MakePlugin([&](Node &r)
							 { Check(r.mId == 'r', "wrong root"); log += 'M'; }, root);
	{
		const auto scope = config.Mount(plugin);
		Check(log == "LABRM", "dependency order");
		const auto selected = scope->GetSelections();
		Check(selected.size() == 4 && selected.front().mSlot == "leaf" && selected.back().mSlot == "root", "construction record");
	}
	Check(log == "LABRMrbal", "reverse teardown");

	log.clear();
	auto failing = config;
	failing.Replace(right.Factory("throw", [](Node &) -> std::unique_ptr<Node>
								  { throw std::runtime_error("failed"); }, leaf));
	ExpectError<std::runtime_error>([&]
									{ (void)failing.Mount(plugin); }, "failed");
	Check(log == "LAal", "partial graph teardown");

	log.clear();
	auto simple = config;
	simple.Replace(root.Factory("standalone", [&]
								{ return std::make_unique<Node>(log, 's', nullptr); }));
	Dependencies request;
	root.Declare(request);
	const auto scope = simple.Build(request);
	const auto selected = scope.GetSelections();
	Check(selected.size() == 1 && selected[0].mImplementation == "standalone" && selected[0].mReplaced && log.empty(), "replacement defines the reachable graph");
}

void CheckGraphErrors()
{
	const CapabilitySlot<int>		a("a"), b("b"), c("c"), independent("independent");
	const CapabilitySlot<const int> readonly("independent");
	int								calls = 0;
	auto							indirect = [&](int &)
	{ ++calls; return std::make_unique<int>(); };
	CapabilityConfig config;
	config.Add(independent.Factory("independent", [&]
								   { ++calls; return std::make_unique<int>(); }));
	config.Add(a.Factory("a", indirect, b));
	config.Add(b.Factory("b", indirect, c));
	Dependencies request;
	independent.Declare(request);
	a.Declare(request);
	ExpectError<std::invalid_argument>([&]
									   { (void)config.Build(request); }, "Missing plugin binding: c required by b");
	config.Add(c.Factory("c", indirect, a));
	ExpectError<std::invalid_argument>([&]
									   { (void)config.Build(request); }, "Plugin dependency cycle: a -> b -> c -> a");
	auto wrong = config;
	wrong.Replace(c.Factory("wrong", [](const int &)
							{ return std::make_unique<int>(); }, readonly));
	ExpectError<std::invalid_argument>([&]
									   { (void)wrong.Build(request); }, "Incompatible plugin binding type: independent required by c");
	Check(calls == 0, "validation must precede all factories");
	Dependencies subset;
	independent.Declare(subset);
	const auto scope = config.Build(subset);
	Check(calls == 1 && scope.GetSelections().size() == 1, "unreachable cycle");
}
} // namespace

int main()
{
	try
	{
		CheckDiamond();
		CheckGraphErrors();
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
