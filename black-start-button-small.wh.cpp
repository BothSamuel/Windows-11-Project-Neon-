// ==WindhawkMod==



// @id              logo-windows-11-black



// @name            Start Logo Black Windows 11



// @version         0.0.0



// @author          KYMA



// @include         explorer.exe



// @architecture    x86-64



// @compilerOptions -lole32 -loleaut32 -lruntimeobject



// ==/WindhawkMod==






#include <windhawk_utils.h>






#undef GetCurrentTime






#include <winrt/Windows.Foundation.Collections.h>



#include <winrt/Windows.Foundation.Numerics.h>



#include <winrt/Windows.Foundation.h>



#include <winrt/Windows.UI.Composition.h>



#include <winrt/Windows.UI.Core.h>



#include <winrt/Windows.UI.Xaml.Automation.h>



#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>



#include <winrt/Windows.UI.Xaml.Controls.h>



#include <winrt/Windows.UI.Xaml.Hosting.h>



#include <winrt/Windows.UI.Xaml.Media.Animation.h>



#include <winrt/Windows.UI.Xaml.Media.h>



#include <winrt/Windows.UI.Xaml.Shapes.h>



#include <winrt/Windows.UI.Xaml.h>






#include <atomic>



#include <chrono>



#include <cstdint>



#include <mutex>



#include <string_view>



#include <utility>



#include <vector>






using namespace winrt::Windows::UI::Xaml;






namespace {






namespace wf = winrt::Windows::Foundation;



namespace wfn = winrt::Windows::Foundation::Numerics;



namespace wux = winrt::Windows::UI::Xaml;



namespace wuxa = winrt::Windows::UI::Xaml::Automation;



namespace wuxc = winrt::Windows::UI::Xaml::Controls;



namespace wuxcp = winrt::Windows::UI::Xaml::Controls::Primitives;



namespace wuxh = winrt::Windows::UI::Xaml::Hosting;



namespace wuxm = winrt::Windows::UI::Xaml::Media;



namespace wuxma = winrt::Windows::UI::Xaml::Media::Animation;



namespace wuxs = winrt::Windows::UI::Xaml::Shapes;






constexpr wchar_t kStartButtonAutomationId[] = L"StartButton";



constexpr wchar_t kRootPanelName[] = L"ExperienceToggleButtonRootPanel";



constexpr wchar_t kNativeIconName[] = L"Icon";



constexpr wchar_t kLogoName[] = L"GalatikaWindows11StartLogo";



constexpr wchar_t kPane1Name[] = L"GalatikaWindows11StartPane1";



constexpr wchar_t kPane2Name[] = L"GalatikaWindows11StartPane2";



constexpr wchar_t kPane3Name[] = L"GalatikaWindows11StartPane3";



constexpr wchar_t kPane4Name[] = L"GalatikaWindows11StartPane4";



constexpr wchar_t kQuantumPane1Name[] =



    L"GalatikaWindows11StartQuantumPane1";



constexpr wchar_t kQuantumPane2Name[] =



    L"GalatikaWindows11StartQuantumPane2";



constexpr wchar_t kQuantumPane3Name[] =



    L"GalatikaWindows11StartQuantumPane3";



constexpr wchar_t kQuantumPane4Name[] =



    L"GalatikaWindows11StartQuantumPane4";






// Le tracÃ© original reste dans son repÃ¨re vectoriel 512 x 512 : quatre carrÃ©s



// de 244 unitÃ©s sÃ©parÃ©s par un espace de 24 unitÃ©s. Un Viewbox l'affiche ensuite



// Ã  23 x 23 px sans modifier ses proportions.



constexpr double kLogoSize = 13.0;



constexpr double kLogoSourceSize = 512.0;



constexpr double kPaneSize = 244.0;



constexpr double kSecondPaneOffset = 268.0;






constexpr float kPressedScale = 0.80f;



constexpr int kPressDurationMilliseconds = 165;



constexpr int kReleaseDurationMilliseconds = 350;



constexpr int kColorDurationMilliseconds = 165;



constexpr int kQuantumPulseDurationMilliseconds = 50;



constexpr int kQuantumPulseStepMilliseconds = 30;






constexpr winrt::Windows::UI::Color kWhite = {0xFF, 0x00, 0x00, 0x00};



constexpr winrt::Windows::UI::Color kGray = {



    0xFF, 0x26, 0x26, 0x26};



constexpr winrt::Windows::UI::Color kQuantumGray = {



    0xFF, 0x60, 0x60, 0x60};






std::atomic<bool> g_taskbarViewDllLoaded = false;



std::atomic<bool> g_unloading = false;






struct NativeButtonState {



    bool active = false;



    bool pointerOver = false;



    bool pressed = false;



};






struct TrackedStartButton {



    winrt::weak_ref<wuxc::Panel> panel;



    winrt::weak_ref<wux::FrameworkElement> nativeIcon;



    wux::Visibility originalNativeIconVisibility = wux::Visibility::Visible;



};






std::mutex g_trackedButtonsMutex;



std::vector<TrackedStartButton> g_trackedButtons;






bool ColorsEqual(const winrt::Windows::UI::Color& left,



                 const winrt::Windows::UI::Color& right) {



    return left.A == right.A && left.R == right.R && left.G == right.G &&



           left.B == right.B;



}






wux::FrameworkElement FindDescendantByName(wux::FrameworkElement element,



                                           std::wstring_view name,



                                           int remainingDepth = 12) {



    if (!element || remainingDepth < 0) {



        return nullptr;



    }






    if (element.Name() == name) {



        return element;



    }






    int childrenCount = 0;



    try {



        childrenCount = wuxm::VisualTreeHelper::GetChildrenCount(element);



    } catch (...) {



        return nullptr;



    }






    for (int i = 0; i < childrenCount; i++) {



        auto child = wuxm::VisualTreeHelper::GetChild(element, i)



                         .try_as<wux::FrameworkElement>();



        if (!child) {



            continue;



        }






        auto result = FindDescendantByName(child, name, remainingDepth - 1);



        if (result) {



            return result;



        }



    }






    return nullptr;



}






wuxs::Rectangle CreatePane(PCWSTR name, double left, double top) {



    wuxs::Rectangle pane;



    pane.Name(name);



    pane.Width(kPaneSize);



    pane.Height(kPaneSize);



    pane.Fill(wuxm::SolidColorBrush(kWhite));



    pane.IsHitTestVisible(false);



    pane.UseLayoutRounding(true);



    wuxc::Canvas::SetLeft(pane, left);



    wuxc::Canvas::SetTop(pane, top);



    return pane;



}



wuxs::Rectangle CreateQuantumPane(PCWSTR name, double left, double top) {



    wuxs::Rectangle pane;



    pane.Name(name);



    pane.Width(kPaneSize);



    pane.Height(kPaneSize);



    pane.Fill(wuxm::SolidColorBrush(kQuantumGray));



    pane.Opacity(0.0);



    pane.IsHitTestVisible(false);



    pane.UseLayoutRounding(true);



    wuxc::Canvas::SetLeft(pane, left);



    wuxc::Canvas::SetTop(pane, top);



    return pane;



}






wuxc::Grid CreateWindows11Logo() {



    wuxc::Grid logo;



    logo.Name(kLogoName);



    logo.Width(kLogoSize);



    logo.Height(kLogoSize);



    logo.HorizontalAlignment(wux::HorizontalAlignment::Center);



    logo.VerticalAlignment(wux::VerticalAlignment::Center);



    logo.IsHitTestVisible(false);



    logo.UseLayoutRounding(true);






    wuxc::Viewbox viewbox;



    viewbox.Width(kLogoSize);



    viewbox.Height(kLogoSize);



    viewbox.HorizontalAlignment(wux::HorizontalAlignment::Center);



    viewbox.VerticalAlignment(wux::VerticalAlignment::Center);



    viewbox.Stretch(wuxm::Stretch::Uniform);



    viewbox.IsHitTestVisible(false);



    viewbox.UseLayoutRounding(true);






    wuxc::Canvas canvas;



    canvas.Width(kLogoSourceSize);



    canvas.Height(kLogoSourceSize);



    canvas.IsHitTestVisible(false);



    canvas.UseLayoutRounding(true);






    canvas.Children().Append(CreatePane(kPane1Name, 0.0, 0.0));



    canvas.Children().Append(



        CreatePane(kPane2Name, kSecondPaneOffset, 0.0));



    canvas.Children().Append(



        CreatePane(kPane3Name, 0.0, kSecondPaneOffset));



    canvas.Children().Append(CreatePane(kPane4Name, kSecondPaneOffset,



                                        kSecondPaneOffset));



    // Calques lumineux utilisés uniquement pendant l'onde quantique.



    canvas.Children().Append(CreateQuantumPane(kQuantumPane1Name, 0.0, 0.0));



    canvas.Children().Append(CreateQuantumPane(



        kQuantumPane2Name, kSecondPaneOffset, 0.0));



    canvas.Children().Append(CreateQuantumPane(



        kQuantumPane3Name, 0.0, kSecondPaneOffset));



    canvas.Children().Append(CreateQuantumPane(



        kQuantumPane4Name, kSecondPaneOffset, kSecondPaneOffset));






    viewbox.Child(canvas);



    logo.Children().Append(viewbox);



    wuxc::Canvas::SetZIndex(logo, 1000);



    return logo;



}






void AnimateBrushColor(const wuxm::SolidColorBrush& brush,



                       const winrt::Windows::UI::Color& target,



                       bool animate,



                       int durationMilliseconds) {



    if (!brush) {



        return;



    }






    auto current = brush.Color();



    if (ColorsEqual(current, target)) {



        return;



    }






    if (!animate) {



        brush.Color(target);



        return;



    }






    // La valeur de base devient immÃ©diatement la valeur finale. Le storyboard



    // affiche l'interpolation puis s'arrÃªte proprement sur cette valeur.



    brush.Color(target);






    wuxma::ColorAnimation animation;



    animation.From(current);



    animation.To(target);



    animation.Duration(wux::DurationHelper::FromTimeSpan(wf::TimeSpan{



        std::chrono::milliseconds(durationMilliseconds)}));



    animation.FillBehavior(wuxma::FillBehavior::Stop);






    wuxma::Storyboard storyboard;



    storyboard.Children().Append(animation);



    wuxma::Storyboard::SetTarget(animation, brush);



    wuxma::Storyboard::SetTargetProperty(animation, L"Color");



    storyboard.Begin();



}






void AnimateLogoScale(const wuxc::Grid& logo,



                      float startScale,



                      float targetScale,



                      bool animate,



                      int durationMilliseconds,



                      const wfn::float2& controlPoint1,



                      const wfn::float2& controlPoint2) {



    auto visual = wuxh::ElementCompositionPreview::GetElementVisual(logo);



    if (!visual) {



        return;



    }






    visual.CenterPoint(



        wfn::float3{static_cast<float>(kLogoSize / 2.0),



                    static_cast<float>(kLogoSize / 2.0), 0.0f});






    wfn::float3 target{targetScale, targetScale, 1.0f};



    if (!animate) {



        visual.StopAnimation(L"Scale");



        visual.Scale(target);



        return;



    }






    auto compositor = visual.Compositor();



    auto easing = compositor.CreateCubicBezierEasingFunction(



        controlPoint1, controlPoint2);



    auto animation = compositor.CreateVector3KeyFrameAnimation();






    // Composition ne modifie pas automatiquement la valeur de base lorsqu'une



    // animation se termine. On fixe donc explicitement la valeur finale et le



    // point de dÃ©part, afin que le relÃ¢chement parte rÃ©ellement de 80 % au lieu



    // d'animer 100 % vers 100 %.



    visual.StopAnimation(L"Scale");



    visual.Scale(target);



    animation.InsertKeyFrame(



        0.0f, wfn::float3{startScale, startScale, 1.0f});



    animation.InsertKeyFrame(1.0f, target, easing);



    animation.Duration(std::chrono::milliseconds(durationMilliseconds));



    visual.StartAnimation(L"Scale", animation);



}



void AnimateQuantumPanePulse(const wux::FrameworkElement& pane,



                             int delayMilliseconds) {



    if (!pane) {



        return;



    }



    auto visual = wuxh::ElementCompositionPreview::GetElementVisual(pane);



    if (!visual) {



        return;



    }



    auto compositor = visual.Compositor();



    auto easing = compositor.CreateCubicBezierEasingFunction(



        wfn::float2{0.12f, 0.75f}, wfn::float2{0.25f, 1.00f});



    auto pulse = compositor.CreateScalarKeyFrameAnimation();



    pulse.InsertKeyFrame(0.00f, 0.00f);



    pulse.InsertKeyFrame(0.22f, 0.92f);



    pulse.InsertKeyFrame(0.52f, 0.46f, easing);



    pulse.InsertKeyFrame(1.00f, 0.00f, easing);



    pulse.DelayTime(std::chrono::milliseconds(delayMilliseconds));



    pulse.Duration(std::chrono::milliseconds(kQuantumPulseDurationMilliseconds));



    visual.StopAnimation(L"Opacity");



    visual.Opacity(0.0f);



    visual.StartAnimation(L"Opacity", pulse);



}



void AnimateQuantumPaneWave(const wuxc::Grid& logo) {



    // Sens horaire : haut-gauche, haut-droite, bas-droite, bas-gauche.



    AnimateQuantumPanePulse(



        FindDescendantByName(logo, kQuantumPane1Name),



        0 * kQuantumPulseStepMilliseconds);



    AnimateQuantumPanePulse(



        FindDescendantByName(logo, kQuantumPane2Name),



        1 * kQuantumPulseStepMilliseconds);



    AnimateQuantumPanePulse(



        FindDescendantByName(logo, kQuantumPane4Name),



        2 * kQuantumPulseStepMilliseconds);



    AnimateQuantumPanePulse(



        FindDescendantByName(logo, kQuantumPane3Name),



        3 * kQuantumPulseStepMilliseconds);



}






NativeButtonState ReadNativeButtonState(const wux::FrameworkElement& panel,



                                        const wux::FrameworkElement& button) {



    NativeButtonState result;






    // Le panneau natif expose exactement les Ã©tats utilisÃ©s par Windows :



    // InactiveNormal, InactivePointerOver, InactivePressed, ActiveNormal,



    // ActivePointerOver et ActivePressed.



    try {



        auto groups = wux::VisualStateManager::GetVisualStateGroups(panel);



        for (const auto& group : groups) {



            if (group.Name() != L"CommonStates") {



                continue;



            }






            auto currentState = group.CurrentState();



            if (!currentState) {



                break;



            }






            auto stateNameString = currentState.Name();



            std::wstring_view stateName{stateNameString.c_str(),



                                        stateNameString.size()};



            result.active = stateName.starts_with(L"Active");



            result.pointerOver = stateName.ends_with(L"PointerOver");



            result.pressed = stateName.ends_with(L"Pressed");



            return result;



        }



    } catch (...) {



        // Le repli ci-dessous couvre les variantes de modÃ¨le XAML.



    }






    try {



        if (auto buttonBase = button.try_as<wuxcp::ButtonBase>()) {



            result.pressed = buttonBase.IsPressed();



        }



        if (auto toggleButton = button.try_as<wuxcp::ToggleButton>()) {



            if (auto checked = toggleButton.IsChecked()) {



                result.active = checked.Value();



            }



        }



    } catch (...) {



    }






    return result;



}






void ApplyLogoState(const wuxc::Grid& logo,



                    const NativeButtonState& state) {



    int32_t previousState = -1;



    if (auto tag = logo.Tag()) {



        previousState = winrt::unbox_value_or<int32_t>(tag, -1);



    }






    // Le bit 3 mÃ©morise l'Ã©tat rÃ©ellement confirmÃ© aprÃ¨s relÃ¢chement. Pendant



    // un appui, Windows peut dÃ©jÃ  annoncer ActivePressed avant que le menu soit



    // visible. On conserve donc le dernier Ã©tat confirmÃ© jusqu'au relÃ¢chement.



    bool menuOpen = previousState >= 0 && (previousState & 8) != 0;



    if (!state.pressed) {



        menuOpen = state.active;



    }






    int32_t newState = (menuOpen ? 8 : 0) |



                       (state.active ? 4 : 0) |



                       (state.pointerOver ? 2 : 0) |



                       (state.pressed ? 1 : 0);






    if (previousState == newState) {



        return;



    }






    logo.Tag(winrt::box_value(newState));



    bool initialized = previousState >= 0;



    bool wasPressed = initialized && (previousState & 1) != 0;



    bool menuJustOpened = initialized &&



                          (previousState & 8) == 0 &&



                          menuOpen && !state.pressed;






    if (!initialized) {



        float initialScale = state.pressed ? kPressedScale : 1.00f;



        AnimateLogoScale(logo, initialScale, initialScale, false, 0, {}, {});



    } else if (wasPressed != state.pressed) {



        if (state.pressed) {



            // RÃ©duction plus marquÃ©e, avec un dÃ©part volontairement plus lent



            // que l'ancienne animation de 75 ms.



            AnimateLogoScale(logo, 1.00f, kPressedScale, true,



                             kPressDurationMilliseconds,



                             wfn::float2{0.33f, 0.00f},



                             wfn::float2{0.67f, 1.00f});



        } else {



            // DÃ©cÃ©lÃ©ration Fluent au relÃ¢chement.



            AnimateLogoScale(logo, kPressedScale, 1.00f, true,



                             kReleaseDurationMilliseconds,



                             wfn::float2{0.10f, 0.90f},



                             wfn::float2{0.20f, 1.00f});



        }



    }



    if (menuJustOpened) {



        AnimateQuantumPaneWave(logo);



    }






    // Le survol du logo ne change pas son Ã©chelle : Windows anime dÃ©jÃ  le fond



    // natif du bouton. Les couleurs suivent uniquement la combinaison demandÃ©e



    // entre l'Ã©tat confirmÃ© du menu et l'appui de la souris.



    auto diagonalColor = kWhite;



    auto otherColor = kWhite;



    if (state.pressed) {



        if (!menuOpen) {



            diagonalColor = kGray;



            otherColor = kGray;



        }



    } else if (menuOpen) {



        diagonalColor = kGray;



    }






    // Pendant l'appui, la couleur est appliquÃ©e directement : elle reste ainsi



    // visible tant que la souris est maintenue. AprÃ¨s relÃ¢chement, la courte



    // transition de couleur existante est conservÃ©e.



    bool animateColors = initialized && !state.pressed;






    auto pane1 = FindDescendantByName(logo, kPane1Name)



                     .try_as<wuxs::Rectangle>();



    auto pane2 = FindDescendantByName(logo, kPane2Name)



                     .try_as<wuxs::Rectangle>();



    auto pane3 = FindDescendantByName(logo, kPane3Name)



                     .try_as<wuxs::Rectangle>();



    auto pane4 = FindDescendantByName(logo, kPane4Name)



                     .try_as<wuxs::Rectangle>();






    if (pane1) {



        AnimateBrushColor(pane1.Fill().try_as<wuxm::SolidColorBrush>(),



                          diagonalColor, animateColors,



                          kColorDurationMilliseconds);



    }



    if (pane2) {



        AnimateBrushColor(pane2.Fill().try_as<wuxm::SolidColorBrush>(),



                          otherColor, animateColors,



                          kColorDurationMilliseconds);



    }



    if (pane3) {



        AnimateBrushColor(pane3.Fill().try_as<wuxm::SolidColorBrush>(),



                          otherColor, animateColors,



                          kColorDurationMilliseconds);



    }



    if (pane4) {



        AnimateBrushColor(pane4.Fill().try_as<wuxm::SolidColorBrush>(),



                          diagonalColor, animateColors,



                          kColorDurationMilliseconds);



    }



}






void TrackStartButton(const wuxc::Panel& panel,



                      const wux::FrameworkElement& nativeIcon,



                      wux::Visibility originalVisibility) {



    void* panelAbi = winrt::get_abi(panel);



    std::lock_guard<std::mutex> lock(g_trackedButtonsMutex);






    for (auto it = g_trackedButtons.begin(); it != g_trackedButtons.end();) {



        auto existingPanel = it->panel.get();



        if (!existingPanel) {



            it = g_trackedButtons.erase(it);



            continue;



        }






        if (winrt::get_abi(existingPanel) == panelAbi) {



            return;



        }



        ++it;



    }






    g_trackedButtons.push_back({winrt::make_weak(panel),



                                winrt::make_weak(nativeIcon),



                                originalVisibility});



}






void ApplyToStartButton(const wux::FrameworkElement& button) {



    if (!button || g_unloading) {



        return;



    }






    if (wuxa::AutomationProperties::GetAutomationId(button) !=



        kStartButtonAutomationId) {



        return;



    }






    auto panel = FindDescendantByName(button, kRootPanelName)



                     .try_as<wuxc::Grid>();



    if (!panel) {



        return;



    }






    auto nativeIcon = FindDescendantByName(panel, kNativeIconName);



    if (!nativeIcon) {



        return;



    }






    auto logo = FindDescendantByName(panel, kLogoName).try_as<wuxc::Grid>();



    if (!logo) {



        auto originalVisibility = nativeIcon.Visibility();



        logo = CreateWindows11Logo();



        TrackStartButton(panel, nativeIcon, originalVisibility);



        nativeIcon.Visibility(wux::Visibility::Collapsed);



        panel.Children().Append(logo);



    } else {



        // Windows peut rÃ©appliquer le modÃ¨le du bouton lors d'un changement



        // d'Ã©tat. Le lecteur animÃ© natif doit rester masquÃ© tant que le mod est



        // actif.



        nativeIcon.Visibility(wux::Visibility::Collapsed);



    }






    ApplyLogoState(logo, ReadNativeButtonState(panel, button));



}






wux::FrameworkElement GetElementFromUpdateVisualStates(void* pThis) {



    try {



        // Disposition interne utilisÃ©e par ExperienceToggleButton sur Windows



        // 11. Ce point d'accÃ¨s est aussi utilisÃ© par les mods Windhawk officiels



        // qui personnalisent la barre des tÃ¢ches.



        void* elementIUnknownPointer = static_cast<void**>(pThis) + 2;



        wf::IUnknown elementIUnknown;



        winrt::copy_from_abi(elementIUnknown, elementIUnknownPointer);



        return elementIUnknown.try_as<wux::FrameworkElement>();



    } catch (...) {



        return nullptr;



    }



}






wux::FrameworkElement GetElementFromUpdateButtonPadding(void* pThis) {



    try {



        IUnknown* elementIUnknown = (static_cast<IUnknown**>(pThis))[1];



        if (!elementIUnknown) {



            return nullptr;



        }






        wux::FrameworkElement element = nullptr;



        elementIUnknown->QueryInterface(winrt::guid_of<wux::FrameworkElement>(),



                                        winrt::put_abi(element));



        return element;



    } catch (...) {



        return nullptr;



    }



}






using ExperienceToggleButton_UpdateVisualStates_t =



    void(WINAPI*)(void* pThis);



ExperienceToggleButton_UpdateVisualStates_t



    ExperienceToggleButton_UpdateVisualStates_Original;






void WINAPI ExperienceToggleButton_UpdateVisualStates_Hook(void* pThis) {



    ExperienceToggleButton_UpdateVisualStates_Original(pThis);






    if (g_unloading) {



        return;



    }






    try {



        ApplyToStartButton(GetElementFromUpdateVisualStates(pThis));



    } catch (...) {



        Wh_Log(L"Mise Ã  jour du logo impossible : 0x%08X",



               winrt::to_hresult());



    }



}






using ExperienceToggleButton_UpdateButtonPadding_t =



    void(WINAPI*)(void* pThis);



ExperienceToggleButton_UpdateButtonPadding_t



    ExperienceToggleButton_UpdateButtonPadding_Original;






void WINAPI ExperienceToggleButton_UpdateButtonPadding_Hook(void* pThis) {



    ExperienceToggleButton_UpdateButtonPadding_Original(pThis);






    if (g_unloading) {



        return;



    }






    try {



        ApplyToStartButton(GetElementFromUpdateButtonPadding(pThis));



    } catch (...) {



        Wh_Log(L"CrÃ©ation du logo impossible : 0x%08X",



               winrt::to_hresult());



    }



}






bool HookTaskbarViewDllSymbols(HMODULE module) {



    WindhawkUtils::SYMBOL_HOOK hooks[] = {



        {



            {



                LR"(private: void __cdecl winrt::Taskbar::implementation::ExperienceToggleButton::UpdateVisualStates(void))",



                LR"(protected: virtual void __cdecl winrt::Taskbar::implementation::ExperienceToggleButton::UpdateVisualStates(void))",



            },



            &ExperienceToggleButton_UpdateVisualStates_Original,



            ExperienceToggleButton_UpdateVisualStates_Hook,



            true,



        },



        {



            {LR"(protected: virtual void __cdecl winrt::Taskbar::implementation::ExperienceToggleButton::UpdateButtonPadding(void))"},



            &ExperienceToggleButton_UpdateButtonPadding_Original,



            ExperienceToggleButton_UpdateButtonPadding_Hook,



            true,



        },



    };






    if (!HookSymbols(module, hooks, ARRAYSIZE(hooks))) {



        Wh_Log(L"Impossible de rÃ©soudre les symboles de la barre des tÃ¢ches");



        return false;



    }






    if (!ExperienceToggleButton_UpdateVisualStates_Original &&



        !ExperienceToggleButton_UpdateButtonPadding_Original) {



        Wh_Log(L"ExperienceToggleButton n'est pas disponible sur cette version");



        return false;



    }






    return true;



}






HMODULE GetTaskbarViewModuleHandle() {



    HMODULE module = GetModuleHandleW(L"Taskbar.View.dll");



    return module ? module : GetModuleHandleW(L"ExplorerExtensions.dll");



}






using LoadLibraryExW_t = decltype(&LoadLibraryExW);



LoadLibraryExW_t LoadLibraryExW_Original;






HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR fileName,



                                   HANDLE file,



                                   DWORD flags) {



    HMODULE module = LoadLibraryExW_Original(fileName, file, flags);



    if (module && !g_taskbarViewDllLoaded &&



        GetTaskbarViewModuleHandle() == module &&



        !g_taskbarViewDllLoaded.exchange(true)) {



        if (HookTaskbarViewDllSymbols(module)) {



            Wh_ApplyHookOperations();



        }



    }






    return module;



}






void RemoveLogoAndRestoreIcon(const TrackedStartButton& trackedButton) {



    auto panel = trackedButton.panel.get();



    if (!panel) {



        return;



    }






    auto weakPanel = trackedButton.panel;



    auto weakNativeIcon = trackedButton.nativeIcon;



    auto originalVisibility = trackedButton.originalNativeIconVisibility;



    auto cleanup = [weakPanel, weakNativeIcon, originalVisibility]() {



        auto currentPanel = weakPanel.get();



        if (!currentPanel) {



            return;



        }






        try {



            auto children = currentPanel.Children();



            for (int i = static_cast<int>(children.Size()) - 1; i >= 0; i--) {



                auto child = children.GetAt(i).try_as<wux::FrameworkElement>();



                if (child && child.Name() == kLogoName) {



                    children.RemoveAt(i);



                }



            }






            if (auto nativeIcon = weakNativeIcon.get()) {



                nativeIcon.Visibility(originalVisibility);



            }



        } catch (...) {



        }



    };






    try {



        auto dispatcher = panel.Dispatcher();



        if (dispatcher.HasThreadAccess()) {



            cleanup();



        } else {



            dispatcher.RunAsync(



                winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,



                cleanup)



                .get();



        }



    } catch (...) {



    }



}






void CleanupAllStartButtons() {



    std::vector<TrackedStartButton> trackedButtons;



    {



        std::lock_guard<std::mutex> lock(g_trackedButtonsMutex);



        trackedButtons = std::move(g_trackedButtons);



    }






    for (const auto& trackedButton : trackedButtons) {



        RemoveLogoAndRestoreIcon(trackedButton);



    }



}






}  // namespace






BOOL Wh_ModInit() {



    Wh_Log(L"Initialisation du logo DÃ©marrer Windows 11");






    if (HMODULE module = GetTaskbarViewModuleHandle()) {



        g_taskbarViewDllLoaded = true;



        return HookTaskbarViewDllSymbols(module);



    }






    HMODULE kernelBase = GetModuleHandleW(L"kernelbase.dll");



    auto loadLibraryExW = reinterpret_cast<decltype(&LoadLibraryExW)>(



        GetProcAddress(kernelBase, "LoadLibraryExW"));



    if (!loadLibraryExW) {



        return FALSE;



    }






    return WindhawkUtils::SetFunctionHook(loadLibraryExW, LoadLibraryExW_Hook,



                                          &LoadLibraryExW_Original);



}






void Wh_ModAfterInit() {



    // Couvre le cas oÃ¹ Taskbar.View.dll est chargÃ© entre Wh_ModInit et ici.



    if (!g_taskbarViewDllLoaded) {



        if (HMODULE module = GetTaskbarViewModuleHandle()) {



            if (!g_taskbarViewDllLoaded.exchange(true) &&



                HookTaskbarViewDllSymbols(module)) {



                Wh_ApplyHookOperations();



            }



        }



    }



}






void Wh_ModBeforeUninit() {



    g_unloading = true;



    CleanupAllStartButtons();



}