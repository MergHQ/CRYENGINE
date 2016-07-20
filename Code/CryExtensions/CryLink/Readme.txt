CryLink is an extension which registers 'CryLink' URI to the OS and allows to handle them from the OS, editor and game.

	* It registers a helper tool (CryLinkHelper) to the OS so links can be opened from explorer, mails etc.
	* It provides an easy way to create links in code and to feed it with commands.
	* A link can contain one or multiple commands

The extension is build as a static library which is supposed to be linked against a Game.dll. The game is also supposed to initialize the service 
but doesn't necessarily run it. Means depending on the mode (game, editor etc.) either the editor extension or the game it self needs to call the update.

More details about the extension, dependencies and its setup can be found in the wiki here:
https://crywiki/display/CryENGINE/FP+Engine+Extension+-+CryLink
