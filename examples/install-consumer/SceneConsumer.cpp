#include <HibikiPhysics/Scene.h>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	using namespace HibikiPhysics;
	const auto			scene = LoadScene(argv[1]);
	auto				config = DefaultSceneConfig();
	const CompiledScene baseline(scene, config);
	config.Replace(SpherePlaneWithMargin("experiment.margin-0.25m.v1", 0.25));
	const CompiledScene experiment(scene, config);
	for (const auto *run : {&baseline, &experiment})
	{
		for (const auto &selection : run->GetSelections())
			std::cout << selection.mSlot << " = " << selection.mImplementation << '\n';
		if (scene.mSphere)
			if (const auto gap = run->QueryContact(scene.mSphere->mCenterZ))
				std::cout << "gap (m): " << *gap << '\n';
	}
	return 0;
}
