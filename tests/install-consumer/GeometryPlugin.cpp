#include <HibikiPhysics/Geometry.h>
#include <iostream>

namespace
{
using namespace HibikiPhysics;

void Check(bool inCondition, const char *inMessage)
{
	if (!inCondition)
		throw std::logic_error(inMessage);
}

void CheckGeometryPlugin()
{
	const auto			  &geometry = GeometryLibrarySlot();
	const GeometryLibrary *injected = nullptr;
	auto				   consumer = MakePlugin([&](GeometryLibrary &ioLibrary)
												 { injected = &ioLibrary; }, geometry);

	CapabilityConfig defaults;
	try
	{
		(void)defaults.Mount(consumer);
		throw std::logic_error("Geometry must be registered in the configuration");
	}
	catch (const std::invalid_argument &error)
	{
		Check(std::string_view(error.what()) == "Missing plugin binding: geometry.library" && !injected, "Missing geometry must be diagnosed before mounting");
	}
	defaults.Add(GeometryLibraryPlugin());
	const auto first = defaults.Mount(consumer);
	Check(injected == &geometry.Get(*first), "Consumer must receive the geometry service selected for its scope");
	const auto selection = first->GetSelections();
	Check(selection.size() == 1 && selection[0].mSlot == "geometry.library" && selection[0].mImplementation == "hibiki.geometry" && !selection[0].mReplaced,
		  "Expected the official geometry library");
	const auto second = defaults.Mount(consumer);
	Check(injected == &geometry.Get(*second) && injected != &geometry.Get(*first), "Each scope must own its geometry library");

	GeometryLibrary external;
	auto			experiment = defaults;
	experiment.Replace(geometry.Borrow("external.geometry", external));
	const auto trial = experiment.Mount(consumer);
	Check(injected == &external && trial->GetSelections()[0].mReplaced && &geometry.Get(*first) != &external, "Experiment replacement must be local");
}
} // namespace

int main()
{
	try
	{
		CheckGeometryPlugin();
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
