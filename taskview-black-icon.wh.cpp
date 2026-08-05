// ==WindhawkMod==
// @id            task-view-icon-black
// @name          Custom Black Task View Icon
// @version       1.0.0
// @author        BothSamuel
// @include       explorer.exe
// @architecture  x86-64
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

// Identificadores para el botón de Vista de Tareas (Task View)
constexpr wchar_t kTaskViewButtonAutomationId[] = L"TaskViewButton";
constexpr wchar_t kRootPanelName[] = L"ExperienceToggleButtonRootPanel";
constexpr wchar_t kNativeIconName[] = L"Icon";
constexpr wchar_t kCustomLogoName[] = L"CustomBlackTaskViewIcon";

constexpr wchar_t kBackCardName[] = L"CustomTaskViewBackCard";
constexpr wchar_t kFrontCardName[] = L"CustomTaskViewFrontCard";

// Dimensiones de renderizado vectorial
constexpr double kIconSize = 20.0;
constexpr double kCanvasSourceSize = 512.0;

// Animaciones de escala al hacer clic
constexpr float kPressedScale = 0.85f;
constexpr int kPressDurationMs = 150;
constexpr int kReleaseDurationMs = 250;
constexpr int kColorDurationMs = 150;

// Paleta de colores monócroma/negra
constexpr winrt::Windows::UI::Color kBlackPure = {0xFF, 0x00, 0x00, 0x00};
constexpr winrt::Windows::UI::Color kBlackHover = {0xFF, 0x2B, 0x2B, 0x2B};
constexpr winrt::Windows::UI::Color kBlackPressed = {0xFF, 0x40, 0x40, 0x40};

std::atomic<bool> g_taskbarViewDllLoaded = false;
std::atomic<bool> g_unloading = false;

struct NativeButtonState {
    bool active = false;
    bool pointerOver = false;
    bool pressed = false;
};

struct TrackedTaskViewButton {
    winrt::weak_ref<wuxc::Panel> panel;
    winrt::weak_ref<wux::FrameworkElement> nativeIcon;
    wux::Visibility originalNativeIconVisibility = wux::Visibility::Visible;
};

std::mutex g_trackedButtonsMutex;
std::vector<TrackedTaskViewButton> g_trackedButtons;

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

// Construye la geometría negra de Task View (dos tarjetas superpuestas)
// Construye la geometría exacta del icono de Task View según la imagen
wuxc::Grid CreateBlackTaskViewIcon() {
    wuxc::Grid rootGrid;
    rootGrid.Name(kCustomLogoName);
    rootGrid.Width(kIconSize);
    rootGrid.Height(kIconSize);
    rootGrid.HorizontalAlignment(wux::HorizontalAlignment::Center);
    rootGrid.VerticalAlignment(wux::VerticalAlignment::Center);
    rootGrid.IsHitTestVisible(false);
    rootGrid.UseLayoutRounding(true);

    wuxc::Viewbox viewbox;
    viewbox.Width(kIconSize);
    viewbox.Height(kIconSize);
    viewbox.HorizontalAlignment(wux::HorizontalAlignment::Center);
    viewbox.VerticalAlignment(wux::VerticalAlignment::Center);
    viewbox.Stretch(wuxm::Stretch::Uniform);
    viewbox.IsHitTestVisible(false);

    wuxc::Canvas canvas;
    canvas.Width(kCanvasSourceSize);
    canvas.Height(kCanvasSourceSize);
    canvas.IsHitTestVisible(false);

    // 1. Tarjeta trasera (Solo contorno grueso en forma de L / tarjeta hueca)
    wuxs::Rectangle backCard;
    backCard.Name(kBackCardName);
    backCard.Width(300.0);
    backCard.Height(220.0);
    backCard.Stroke(wuxm::SolidColorBrush(kBlackPure));
    backCard.StrokeThickness(36.0); // Ancho del contorno
    backCard.Fill(wuxm::SolidColorBrush(kBlackPure)); // Sin relleno interno
    backCard.IsHitTestVisible(false);
    wuxc::Canvas::SetLeft(backCard, 60.0);
    wuxc::Canvas::SetTop(backCard, 100.0);

// 2. Tarjeta frontal (Sólida con borde de corte para fondo claro)
    wuxs::Rectangle frontCard;
    frontCard.Name(kFrontCardName);
    frontCard.Width(300.0);
    frontCard.Height(220.0);
    frontCard.Fill(wuxm::SolidColorBrush(kBlackPure));
    
    // Borde blanco fijo para crear el espacio/recorte sobre la barra de tareas clara
    constexpr winrt::Windows::UI::Color kWhiteCutout = {0xFF, 0xFF, 0xFF, 0xFF};
    frontCard.Stroke(wuxm::SolidColorBrush(kWhiteCutout));
    frontCard.StrokeThickness(24.0); // Ancho del espacio transparente/blanco
    
    frontCard.IsHitTestVisible(false);
    wuxc::Canvas::SetLeft(frontCard, 150.0);
    wuxc::Canvas::SetTop(frontCard, 180.0);

    canvas.Children().Append(backCard);
    canvas.Children().Append(frontCard);

    viewbox.Child(canvas);
    rootGrid.Children().Append(viewbox);
    wuxc::Canvas::SetZIndex(rootGrid, 1000);

    return rootGrid;
}

void AnimateIconScale(const wuxc::Grid& icon,
                      float startScale,
                      float targetScale,
                      bool animate,
                      int durationMilliseconds) {
    auto visual = wuxh::ElementCompositionPreview::GetElementVisual(icon);
    if (!visual) {
        return;
    }
    visual.CenterPoint(wfn::float3{static_cast<float>(kIconSize / 2.0),
                                   static_cast<float>(kIconSize / 2.0), 0.0f});
    wfn::float3 target{targetScale, targetScale, 1.0f};

    if (!animate) {
        visual.StopAnimation(L"Scale");
        visual.Scale(target);
        return;
    }

    auto compositor = visual.Compositor();
    auto easing = compositor.CreateCubicBezierEasingFunction(
        wfn::float2{0.10f, 0.90f}, wfn::float2{0.20f, 1.00f});
    auto animation = compositor.CreateVector3KeyFrameAnimation();

    visual.StopAnimation(L"Scale");
    visual.Scale(target);
    animation.InsertKeyFrame(0.0f, wfn::float3{startScale, startScale, 1.0f});
    animation.InsertKeyFrame(1.0f, target, easing);
    animation.Duration(std::chrono::milliseconds(durationMilliseconds));
    visual.StartAnimation(L"Scale", animation);
}

NativeButtonState ReadNativeButtonState(const wux::FrameworkElement& panel,
                                        const wux::FrameworkElement& button) {
    NativeButtonState result;
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
            std::wstring_view stateName{stateNameString.c_str(), stateNameString.size()};

            result.active = stateName.starts_with(L"Active");
            result.pointerOver = stateName.ends_with(L"PointerOver");
            result.pressed = stateName.ends_with(L"Pressed");
            return result;
        }
    } catch (...) {}

    try {
        if (auto buttonBase = button.try_as<wuxcp::ButtonBase>()) {
            result.pressed = buttonBase.IsPressed();
        }
        if (auto toggleButton = button.try_as<wuxcp::ToggleButton>()) {
            if (auto checked = toggleButton.IsChecked()) {
                result.active = checked.Value();
            }
        }
    } catch (...) {}

    return result;
}

void ApplyIconState(const wuxc::Grid& icon, const NativeButtonState& state) {
    int32_t previousState = -1;
    if (auto tag = icon.Tag()) {
        previousState = winrt::unbox_value_or<int32_t>(tag, -1);
    }

    int32_t newState = (state.active ? 4 : 0) |
                       (state.pointerOver ? 2 : 0) |
                       (state.pressed ? 1 : 0);

    if (previousState == newState) {
        return;
    }
    icon.Tag(winrt::box_value(newState));

    bool initialized = previousState >= 0;
    bool wasPressed = initialized && (previousState & 1) != 0;

    if (!initialized) {
        float initialScale = state.pressed ? kPressedScale : 1.00f;
        AnimateIconScale(icon, initialScale, initialScale, false, 0);
    } else if (wasPressed != state.pressed) {
        if (state.pressed) {
            AnimateIconScale(icon, 1.00f, kPressedScale, true, kPressDurationMs);
        } else {
            AnimateIconScale(icon, kPressedScale, 1.00f, true, kReleaseDurationMs);
        }
    }

    // Cambiar el tono del negro según la interacción
    winrt::Windows::UI::Color targetColor = kBlackPure;
    if (state.pressed) {
        targetColor = kBlackPressed;
    } else if (state.pointerOver || state.active) {
        targetColor = kBlackHover;
    }

    auto backCard = FindDescendantByName(icon, kBackCardName).try_as<wuxs::Rectangle>();
    auto frontCard = FindDescendantByName(icon, kFrontCardName).try_as<wuxs::Rectangle>();

    if (backCard && frontCard) {
        // Solo cambia el color del trazo en la tarjeta trasera
        backCard.Stroke(wuxm::SolidColorBrush(targetColor));
        
        // Solo cambia el relleno en la tarjeta frontal (mantiene el borde de corte intacto)
        frontCard.Fill(wuxm::SolidColorBrush(targetColor));
    }
}

void TrackTaskViewButton(const wuxc::Panel& panel,
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

void ApplyToTaskViewButton(const wux::FrameworkElement& button) {
    if (!button || g_unloading) {
        return;
    }

    // Filtra exclusivamente para el botón de Vista de Tareas
    if (wuxa::AutomationProperties::GetAutomationId(button) != kTaskViewButtonAutomationId) {
        return;
    }

    auto panel = FindDescendantByName(button, kRootPanelName).try_as<wuxc::Grid>();
    if (!panel) return;

    auto nativeIcon = FindDescendantByName(panel, kNativeIconName);
    if (!nativeIcon) return;

    auto customIcon = FindDescendantByName(panel, kCustomLogoName).try_as<wuxc::Grid>();
    if (!customIcon) {
        auto originalVisibility = nativeIcon.Visibility();
        customIcon = CreateBlackTaskViewIcon();
        TrackTaskViewButton(panel, nativeIcon, originalVisibility);
        nativeIcon.Visibility(wux::Visibility::Collapsed);
        panel.Children().Append(customIcon);
    } else {
        nativeIcon.Visibility(wux::Visibility::Collapsed);
    }

    ApplyIconState(customIcon, ReadNativeButtonState(panel, button));
}

wux::FrameworkElement GetElementFromUpdateVisualStates(void* pThis) {
    try {
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
        if (!elementIUnknown) return nullptr;
        wux::FrameworkElement element = nullptr;
        elementIUnknown->QueryInterface(winrt::guid_of<wux::FrameworkElement>(),
                                         winrt::put_abi(element));
        return element;
    } catch (...) {
        return nullptr;
    }
}

using ExperienceToggleButton_UpdateVisualStates_t = void(WINAPI*)(void* pThis);
ExperienceToggleButton_UpdateVisualStates_t ExperienceToggleButton_UpdateVisualStates_Original;

void WINAPI ExperienceToggleButton_UpdateVisualStates_Hook(void* pThis) {
    ExperienceToggleButton_UpdateVisualStates_Original(pThis);
    if (g_unloading) return;
    try {
        ApplyToTaskViewButton(GetElementFromUpdateVisualStates(pThis));
    } catch (...) {}
}

using ExperienceToggleButton_UpdateButtonPadding_t = void(WINAPI*)(void* pThis);
ExperienceToggleButton_UpdateButtonPadding_t ExperienceToggleButton_UpdateButtonPadding_Original;

void WINAPI ExperienceToggleButton_UpdateButtonPadding_Hook(void* pThis) {
    ExperienceToggleButton_UpdateButtonPadding_Original(pThis);
    if (g_unloading) return;
    try {
        ApplyToTaskViewButton(GetElementFromUpdateButtonPadding(pThis));
    } catch (...) {}
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

HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR fileName, HANDLE file, DWORD flags) {
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

void RemoveIconAndRestoreNative(const TrackedTaskViewButton& trackedButton) {
    auto panel = trackedButton.panel.get();
    if (!panel) return;

    auto weakPanel = trackedButton.panel;
    auto weakNativeIcon = trackedButton.nativeIcon;
    auto originalVisibility = trackedButton.originalNativeIconVisibility;

    auto cleanup = [weakPanel, weakNativeIcon, originalVisibility]() {
        auto currentPanel = weakPanel.get();
        if (!currentPanel) return;
        try {
            auto children = currentPanel.Children();
            for (int i = static_cast<int>(children.Size()) - 1; i >= 0; i--) {
                auto child = children.GetAt(i).try_as<wux::FrameworkElement>();
                if (child && child.Name() == kCustomLogoName) {
                    children.RemoveAt(i);
                }
            }
            if (auto nativeIcon = weakNativeIcon.get()) {
                nativeIcon.Visibility(originalVisibility);
            }
        } catch (...) {}
    };

    try {
        auto dispatcher = panel.Dispatcher();
        if (dispatcher.HasThreadAccess()) {
            cleanup();
        } else {
            dispatcher.RunAsync(
                winrt::Windows::UI::Core::CoreDispatcherPriority::Normal, cleanup).get();
        }
    } catch (...) {}
}

void CleanupAllTaskViewButtons() {
    std::vector<TrackedTaskViewButton> trackedButtons;
    {
        std::lock_guard<std::mutex> lock(g_trackedButtonsMutex);
        trackedButtons = std::move(g_trackedButtons);
    }
    for (const auto& trackedButton : trackedButtons) {
        RemoveIconAndRestoreNative(trackedButton);
    }
}

}  // namespace

BOOL Wh_ModInit() {
    if (HMODULE module = GetTaskbarViewModuleHandle()) {
        g_taskbarViewDllLoaded = true;
        return HookTaskbarViewDllSymbols(module);
    }

    HMODULE kernelBase = GetModuleHandleW(L"kernelbase.dll");
    auto loadLibraryExW = reinterpret_cast<decltype(&LoadLibraryExW)>(
        GetProcAddress(kernelBase, "LoadLibraryExW"));
    if (!loadLibraryExW) return FALSE;

    return WindhawkUtils::SetFunctionHook(loadLibraryExW, LoadLibraryExW_Hook,
                                          &LoadLibraryExW_Original);
}

void Wh_ModAfterInit() {
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
    CleanupAllTaskViewButtons();
}