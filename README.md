# Field Change Log

Records **when individual fields change**, not just when the whole piece of
content was last saved, and shows it on the page, in a block and in Views.

For example: a review site can show "Comments last updated: October 5, 2026"
under a reviewer's comments, even when other fields on the same content (a
score, tags, an image) were edited more recently. A directory can show when a
listing's phone number or opening hours last changed. A list of products can
be sorted by when their prices last changed.

Only the **date and person** of each change are recorded, not the old values.
To keep old values, use content revisions.

## Usage

1. Enable the module.
2. Edit a field under **Manage Fields** and check **Track when this field
   changes**. Works for any entity type with fields (content, people,
   taxonomy terms, custom entities).
3. Changes are recorded from then on, each time the field's stored value
   changes. Saving without changing the field, or changing other fields,
   records nothing. Field-only saves by other modules are caught too.
4. Show the date in any of these ways:
   - **Manage Display:** each tracked field gets a "*Field*: last changed"
     item, hidden at first. Drag it into place.
   - **Layouts:** the **Field last changed** block shows it for the current
     content, with your own text before the date, a date format, and
     optionally who made the change.
   - **Views:** in the "Field Change Log" group, each tracked field has a
     "last changed" date (field, sort and filter) and a "last changed by"
     relationship.

## Filling in past changes

Tracking starts when you turn it on. For content that changed earlier, go to
**Configuration > Content authoring > Field Change Log** and use **Fill in
past changes**:

- If the content has **revisions**, each revision in which the field changed
  is recorded, with that revision's date and author. Edits saved without a
  new revision keep the older revision's date, so for the most reliable
  history, create new revisions by default.
- If it has no revision history, you can choose to use the content's
  "updated" date instead. That date is approximate: it may be later than the
  field's real last change.

Content that already has recorded changes is skipped, so it is safe to run
again. Only content (nodes) keeps revisions; other entity types start from
when tracking was turned on.

## The admin page

**Configuration > Content authoring > Field Change Log** lists the tracked
fields with how many changes have been recorded and the latest one.

## Notes

- **What counts as a change:** the field's stored values. For text fields,
  differences in spacing or line breaks alone are ignored, because editors
  such as CKEditor reformat HTML when content is saved. Changed words or
  markup always count.
- **Access:** the date is only shown to people who can see the field itself.
  Views can't check field access per row, so only add a "last changed" date to
  Views that list fields people are allowed to see.
- **Turning tracking off and on again:** changes made while a field is not
  tracked are not recorded, so the date shown afterwards may be older than
  the real last change.
- **Display modes:** the "last changed" item starts hidden in the view modes
  that exist when tracking is turned on. A view mode given its own settings
  later shows it until you hide it there.
- **Listings:** each displayed item with a visible "last changed" makes one
  small database query.
- Each language of a translated field is tracked separately. In Views, a
  field tracked in several languages can produce one row per language.
- Deleting content deletes its recorded changes. Deleting a field deletes the
  changes recorded for it.
- Developers: see `field_change_log.api.php` for the functions other modules
  can use.

## Requirements

Backdrop CMS 1.x. Field module (core).

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory
for complete text.

## Current Maintainer

Tim Erickson (https://github.com/stpaultim)
