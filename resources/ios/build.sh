#!/bin/sh

echo "iOS automation started!"
echo

echo "Packing assets..."
PACK="./SDK/macos/suika3-pack";
xattr -c "$PACK";
chmod +x "$PACK";
if "$PACK" $(ls -A | grep -vE '^(SDK|assets.arc|suika3.exe|.vscode)$'); then
    echo "assets.arc ok";
else
    echo "Error: Failed to make assets.arc.";
    exit 1;
fi;
mv assets.arc SDK/ios/
cd SDK/ios/
echo "Done."
echo

echo "Building app..."
xcodebuild archive -scheme ios -archivePath app.xcarchive
IDENTIFIER=$(grep PRODUCT_BUNDLE_IDENTIFIER ios.xcodeproj/project.pbxproj | head -n1 | sed -e 's/.*PRODUCT_BUNDLE_IDENTIFIER = "\(.*\)";$/\1/')
echo "Done."
echo
echo "Bundle Identifier: $IDENTIFIER"
echo

echo "Archiving IPA file..."
xcodebuild -exportArchive -archivePath app.xcarchive -exportPath ./output -exportOptionsPlist ExportOptions.plist
echo "Done."
echo

Echo "Here is the list of iOS deviced registered on this Mac."
xcrun devicectl list devices
DEVICE=$(xcrun devicectl list devices | grep 'available (paired)' | sed -e 's/.*\([0-9A-F]\{8\}-[0-9A-F]\{4\}-[0-9A-F]\{4\}-[0-9A-F]\{4\}-[0-9A-F]\{12\}\).*/\1/')
if [ -z $DEVICE ]; then
	echo "iOS device not available.";
	exit 1;
fi
echo "Done."
echo

echo "Installing the app..."
xcrun devicectl device install app --device "$DEVICE" ./app.xcarchive/Products/Applications/*.app
echo "Done."
echo

echo "Launching..."
xcrun devicectl device process launch --device "$DEVICE" "$IDENTIFIER"
echo "Done."
echo
