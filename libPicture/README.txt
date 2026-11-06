Integration du dialogue JPEG moderne
=====================================

Fichiers:
- JpegExportDialogModern.h
- JpegExportDialogModern.cpp

Le dialogue construit son interface en C++ avec les composants custom de
RegardsModernControls.h/.cpp. Il ne charge pas le XRC précédent.

Ajouter les quatre fichiers .h/.cpp à votre projet:
- RegardsModernControls.h / .cpp
- JpegExportDialogModern.h / .cpp

Exemple d'utilisation:

    Regards::UI::CJpegExportDialogModern dlg(
        parent,
        originalWxImage,
        jpegPreviewWxImage,
        90,
        Regards::UI::CJpegExportDialogModern::Baseline,
        [this](int quality, int codec)
        {
            // Recalculer l'aperçu JPEG avec vos fonctions existantes.
            // Mettre à jour ensuite dlg.SetJpegPreview(...).
        });

    if (dlg.ShowModal() == wxID_OK)
    {
        const int quality = dlg.GetQuality();
        const int codec = dlg.GetCodecOption();

        // Appeler votre fonction d'export JPEG existante.
    }

Important:
- L'argument jpegPreviewWxImage peut être une image vide.
- Le texte de taille JPEG est une estimation mémoire (non la taille du fichier
  compressé). Remplacez-le après encodage si vous avez la taille réelle.
- Les indices CodecOption sont 0..8 et suivent l'ordre des libellés.
- Cette première intégration utilise des contrôles custom et des sizers C++,
  pas le fichier XRC. Cela évite d'écrire des handlers XRC custom.
- Vérifier les options JPEG dans votre encodeur : les libellés doivent correspondre
  à la sémantique de votre codec.
