#include "PluginEditor.h"

SpatialPannerEditor::SpatialPannerEditor (SpatialPannerProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (420, 460);
    curX = *proc.apvts.getRawParameterValue ("x");
    curY = *proc.apvts.getRawParameterValue ("y");
    startTimerHz (30);   // suit l'automation de l'hote
}

juce::Point<float> SpatialPannerEditor::centre() const
{
    return { getWidth() * 0.5f, 60.0f + (getHeight() - 60.0f) * 0.5f };
}

float SpatialPannerEditor::padRadius() const
{
    return std::min ((float) getWidth() * 0.5f - 40.0f, ((float) getHeight() - 60.0f) * 0.5f - 30.0f);
}

juce::Point<float> SpatialPannerEditor::getDotPosition() const
{
    auto c = centre();
    const float R = padRadius();
    return { c.x + curX * R, c.y - curY * R };   // haut = devant
}

void SpatialPannerEditor::timerCallback()
{
    const float x = *proc.apvts.getRawParameterValue ("x");
    const float y = *proc.apvts.getRawParameterValue ("y");
    if (x != curX || y != curY)
    {
        curX = x; curY = y;
        repaint();
    }
}

void SpatialPannerEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15171c));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("CASQUE SPATIAL 3D", 0, 12, getWidth(), 28, juce::Justification::centred);

    const auto c = centre();
    const float R = padRadius();

    // Zone circulaire + anneaux
    g.setColour (juce::Colour (0xff1f232b));
    g.fillEllipse (c.x - R, c.y - R, 2 * R, 2 * R);
    g.setColour (juce::Colour (0xff3a404c));
    for (float k : { 1.0f, 0.66f, 0.33f })
        g.drawEllipse (c.x - R * k, c.y - R * k, 2 * R * k, 2 * R * k, 1.0f);
    g.drawLine (c.x - R, c.y, c.x + R, c.y, 1.0f);
    g.drawLine (c.x, c.y - R, c.x, c.y + R, 1.0f);

    // Libelles
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText ("AVANT",    (int) c.x - 40, (int) (c.y - R) - 22, 80, 18, juce::Justification::centred);
    g.drawText ("ARRIERE",  (int) c.x - 40, (int) (c.y + R) + 4,  80, 18, juce::Justification::centred);
    g.drawText ("G", (int) (c.x - R) - 26, (int) c.y - 9, 22, 18, juce::Justification::centred);
    g.drawText ("D", (int) (c.x + R) + 4,  (int) c.y - 9, 22, 18, juce::Justification::centred);

    // ---- Casque au milieu ----
    const float hr = 34.0f;
    g.setColour (juce::Colours::white);

    juce::Path band;   // arceau
    band.addCentredArc (c.x, c.y, hr, hr * 1.1f, 0.0f,
                        -juce::MathConstants<float>::halfPi * 1.05f,
                         juce::MathConstants<float>::halfPi * 1.05f, true);
    g.strokePath (band, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xffd8dde8));   // ecouteurs
    g.fillRoundedRectangle (c.x - hr - 11.0f, c.y - 5.0f, 22.0f, 38.0f, 8.0f);
    g.fillRoundedRectangle (c.x + hr - 11.0f, c.y - 5.0f, 22.0f, 38.0f, 8.0f);
    g.setColour (juce::Colour (0xff15171c));
    g.fillRoundedRectangle (c.x - hr - 6.0f, c.y + 2.0f, 12.0f, 24.0f, 5.0f);
    g.fillRoundedRectangle (c.x + hr - 6.0f, c.y + 2.0f, 12.0f, 24.0f, 5.0f);

    // petit nez pour montrer l'avant
    juce::Path nose;
    nose.addTriangle (c.x - 6.0f, c.y - 8.0f, c.x + 6.0f, c.y - 8.0f, c.x, c.y - 20.0f);
    g.setColour (juce::Colour (0xff3a404c));
    g.fillPath (nose);

    // ---- Point rouge ----
    const auto d = getDotPosition();
    g.setColour (juce::Colours::red.withAlpha (0.25f));
    g.fillEllipse (d.x - 20.0f, d.y - 20.0f, 40.0f, 40.0f);
    g.setColour (juce::Colours::red);
    g.fillEllipse (d.x - 11.0f, d.y - 11.0f, 22.0f, 22.0f);
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.drawEllipse (d.x - 11.0f, d.y - 11.0f, 22.0f, 22.0f, 1.5f);

    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Double-clic : recentrer  |  Mets un casque !", 0, getHeight() - 22, getWidth(), 18,
                juce::Justification::centred);
}

void SpatialPannerEditor::setPosition (float x, float y)
{
    auto* px = proc.apvts.getParameter ("x");
    auto* py = proc.apvts.getParameter ("y");
    px->setValueNotifyingHost (px->convertTo0to1 (x));
    py->setValueNotifyingHost (py->convertTo0to1 (y));
    curX = x; curY = y;
    repaint();
}

void SpatialPannerEditor::setFromMouse (juce::Point<float> p)
{
    const auto c = centre();
    const float R = padRadius();
    float x = (p.x - c.x) / R;
    float y = -(p.y - c.y) / R;
    const float len = std::sqrt (x * x + y * y);
    if (len > 1.0f) { x /= len; y /= len; }   // reste dans le cercle
    setPosition (x, y);
}

void SpatialPannerEditor::mouseDown (const juce::MouseEvent& e)
{
    proc.apvts.getParameter ("x")->beginChangeGesture();
    proc.apvts.getParameter ("y")->beginChangeGesture();
    setFromMouse (e.position);
}

void SpatialPannerEditor::mouseDrag (const juce::MouseEvent& e) { setFromMouse (e.position); }

void SpatialPannerEditor::mouseUp (const juce::MouseEvent&)
{
    proc.apvts.getParameter ("x")->endChangeGesture();
    proc.apvts.getParameter ("y")->endChangeGesture();
}

void SpatialPannerEditor::mouseDoubleClick (const juce::MouseEvent&) { setPosition (0.0f, 0.0f); }
