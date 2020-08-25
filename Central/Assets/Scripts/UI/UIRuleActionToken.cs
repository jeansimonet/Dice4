﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class UIRuleActionToken : MonoBehaviour
{
    [Header("Controls")]
    public UIParameterEnum actionSelector;
    public RectTransform parametersRoot;
    public Text labelText;
    public Button deleteButton;

    public Behaviors.EditRule parentRule { get; private set; }
    public Behaviors.EditAction editAction { get; private set; }
    public Button.ButtonClickedEvent onDelete => deleteButton.onClick;

    UIParameterManager.ObjectParameterList parameters;

    void OnDestroy()
    {
        foreach (var parameter in parameters.parameters)
        {
            GameObject.Destroy(parameter.gameObject);
        }
        parameters = null;
    }

    public void Setup(Behaviors.EditRule rule, Behaviors.EditAction action, bool first)
    {
        parentRule = rule;
        editAction = action;
        labelText.text = first ? "Then" : "And";
        actionSelector.Setup("Action Type", () => editAction.type, (t) => SetActionType((Behaviors.ActionType)t));

        // Setup all other parameters
        parameters = UIParameterManager.Instance.CreateControls(action, parametersRoot);
    }

    void SetActionType(Behaviors.ActionType newType)
    {
        if (newType != editAction.type)
        {
            // Change the type, which really means create a new action and replace the old one
            var newAction = Behaviors.EditAction.Create(newType);

            // Replace the action
            parentRule.ReplaceAction(editAction, newAction);

            // Setup the parameters again
            foreach (var parameter in parameters.parameters)
            {
                GameObject.Destroy(parameter.gameObject);
            }
            parameters = UIParameterManager.Instance.CreateControls(newAction, parametersRoot);

            editAction = newAction;
        }
    }
}
