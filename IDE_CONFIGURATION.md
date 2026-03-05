# STM32CubeIDE Configuration Guide

## 🎯 Quick Setup (5 Minutes)

Follow these steps to configure your IDE for the new structure:

---

## Step 1: Update Include Paths

1. **Right-click** on your project → **Properties**
2. Navigate to: **C/C++ General** → **Paths and Symbols**
3. Click the **Includes** tab
4. Select **GNU C** (and **GNU C++** if available)
5. Click **Add...** button

Add these paths **one by one**:

```
${workspace_loc:/${ProjName}/App/Inc}
${workspace_loc:/${ProjName}/App/Inc/HAL}
${workspace_loc:/${ProjName}/App/Inc/Tasks}
${workspace_loc:/${ProjName}/App/Inc/Drivers}
```

✅ Check the box **"Add to all configurations"**  
✅ Check the box **"Add to all languages"**

---

## Step 2: Update Source Locations

1. Still in **Properties**, navigate to: **C/C++ General** → **Paths and Symbols**
2. Click the **Source Location** tab
3. You should see your project root
4. Click **Add Folder...**

Add these source folders:

```
App/Src
App/Src/HAL
App/Src/Tasks
App/Src/Drivers
```

For each folder:
- Browse to the folder
- Click **OK**
- Ensure it shows in the list

---

## Step 3: Remove Old Source Paths (Optional Cleanup)

If you still see these old paths, **remove them**:
- `App/HAL`
- `App/Tasks`
- `App/Source`
- `App/Drivers`

**Select** the old path → Click **Remove**

---

## Step 4: Clean and Rebuild

1. **Project** → **Clean...**
2. Select **Clean all projects**
3. Click **Clean**
4. **Project** → **Build All** (or press Ctrl+B)

---

## Step 5: Verify Configuration

After building, you should see:
- ✅ **0 Errors**
- ✅ **0 Warnings** (or only minor warnings)
- ✅ All files compiled successfully

Check the **Console** tab for build output.

---

## 📋 Troubleshooting

### Problem: "No such file or directory" errors

**Solution:** Check your include paths are correct. Make sure all 4 paths were added:
- App/Inc
- App/Inc/HAL  
- App/Inc/Tasks
- App/Inc/Drivers

### Problem: "Multiple definition" errors

**Solution:** Make sure old source folders are removed from build.
1. Properties → C/C++ General → Paths and Symbols → Source Location
2. Remove: App/HAL, App/Tasks, App/Source, App/Drivers

### Problem: Files not compiling

**Solution:** 
1. Right-click the project → **Index** → **Rebuild**
2. Clean and rebuild the project
3. Close and reopen STM32CubeIDE

### Problem: Can't find app_init.h

**Solution:** Make sure `App/Inc` is in your include paths

---

## 🔍 Visual Guide

### Include Paths Dialog
```
┌─────────────────────────────────────────┐
│ Includes (GNU C)                        │
├─────────────────────────────────────────┤
│ ✓ ${workspace_loc:/${ProjName}/App/Inc}           │
│ ✓ ${workspace_loc:/${ProjName}/App/Inc/HAL}       │
│ ✓ ${workspace_loc:/${ProjName}/App/Inc/Tasks}     │
│ ✓ ${workspace_loc:/${ProjName}/App/Inc/Drivers}   │
│   [...other system includes...]         │
└─────────────────────────────────────────┘
```

### Source Location Dialog
```
┌─────────────────────────────────────────┐
│ Source folders:                         │
├─────────────────────────────────────────┤
│ ✓ /High_VoltageTeam                    │
│ ✓ /High_VoltageTeam/App/Src            │
│ ✓ /High_VoltageTeam/App/Src/HAL        │
│ ✓ /High_VoltageTeam/App/Src/Tasks      │
│ ✓ /High_VoltageTeam/App/Src/Drivers    │
│ ✓ /High_VoltageTeam/Core/Src           │
│   [...other core folders...]            │
└─────────────────────────────────────────┘
```

---

## ✅ Verification Checklist

After configuration, verify:

- [ ] Project builds successfully (0 errors)
- [ ] No "file not found" errors
- [ ] No "multiple definition" errors  
- [ ] All .c files in App/Src/* folders are included in build
- [ ] Index is up to date (Ctrl+click on a function works)

---

## 🎓 Tips

### Tip 1: Use Workspace Variables
The `${workspace_loc:/${ProjName}/...}` syntax makes paths portable across different computers.

### Tip 2: Check Exclusions
If a file isn't building:
1. Right-click the file → **Properties**
2. Check **C/C++ Build** → **Settings**
3. Ensure "Exclude resource from build" is **unchecked**

### Tip 3: Rebuild Index
If autocomplete stops working:
- **Project** → **C/C++ Index** → **Rebuild**

---

## 📞 Need Help?

If configuration issues persist:
1. Close STM32CubeIDE
2. Delete the `.settings` folder in your project
3. Reopen IDE and reconfigure
4. Or create a new project and copy the working configuration

---

**Configuration Version:** 2.1.0
**Last Updated:** February 7, 2026
